// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#ifndef WITH_FREETYPE
	#define WITH_FREETYPE	0
#endif // WITH_FREETYPE


#if PLATFORM_COMPILER_HAS_GENERIC_KEYWORD
	#define generic __identifier(generic)
#endif	//PLATFORM_COMPILER_HAS_GENERIC_KEYWORD


#if WITH_FREETYPE
	#include "ft2build.h"

	// FreeType style include
	#include FT_FREETYPE_H
	#include FT_GLYPH_H
	#include FT_MODULE_H
	#include FT_BITMAP_H
	#include FT_ADVANCES_H
	#include FT_STROKER_H
#endif // WITH_FREETYPE


#if PLATFORM_COMPILER_HAS_GENERIC_KEYWORD
	#undef generic
#endif	//PLATFORM_COMPILER_HAS_GENERIC_KEYWORD


namespace FreeTypeConstants
{
	/** The horizontal DPI we render at */
	const uint32 HorizontalDPI = 96;
	/** The vertical DPI we render at */
	const uint32 VerticalDPI = 96;
} // namespace FreeTypeConstants


namespace FreeTypeUtils
{

#if WITH_FREETYPE

/** Apply the given point size and scale to the face */
void ApplySizeAndScale(FT_Face InFace, const int32 InFontSize, const float InFontScale);

/** Load the given glyph into the active slot of the given face */
FT_Error LoadGlyph(FT_Face InFace, const uint32 InGlyphIndex, const int32 InLoadFlags, const int32 InFontSize, const float InFontScale);

#endif // WITH_FREETYPE

/** Convert the given value from 26.6 space into rounded pixel space */
template <typename TRetType, typename TParamType>
FORCEINLINE TRetType Convert26Dot6ToRoundedPixel(TParamType InValue)
{
	return static_cast<TRetType>(FMath::RoundToInt(InValue / 64.0f));
}

/** Convert the given value from pixel space into 26.6 space */
template <typename TRetType, typename TParamType>
FORCEINLINE TRetType ConvertPixelTo26Dot6(TParamType InValue)
{
	return static_cast<TRetType>(InValue * 64);
}

/** Convert the given value from pixel space into 16.16 space */
template <typename TRetType, typename TParamType>
FORCEINLINE TRetType ConvertPixelTo16Dot16(TParamType InValue)
{
	return static_cast<TRetType>(InValue * 65536);
}

}


/** 
 * Wrapper around a FreeType library instance.
 * This instance will be created using our memory allocator. 
 */
class FFreeTypeLibrary
{
public:
	FFreeTypeLibrary();
	~FFreeTypeLibrary();

#if WITH_FREETYPE
	FORCEINLINE FT_Library GetLibrary() const
	{
		return FTLibrary;
	}
#endif // WITH_FREETYPE

private:
	// Non-copyable
	FFreeTypeLibrary(const FFreeTypeLibrary&);
	FFreeTypeLibrary& operator=(const FFreeTypeLibrary&);

#if WITH_FREETYPE
	FT_Library FTLibrary;
	FT_Memory CustomMemory;
#endif // WITH_FREETYPE
};


/**
 * Wrapper around a FreeType face instance.
 * Will hold on to a copy of the memory buffer given in the constructor (this buffer was used to create the FT_Face).
 */
class FFreeTypeFace
{
public:
	FFreeTypeFace(const FFreeTypeLibrary* InFTLibrary, const void* InRawFontData, const int32 InRawFontDataSizeBytes);
	~FFreeTypeFace();

	FORCEINLINE bool IsValid() const
	{
#if WITH_FREETYPE
		return FTFace != nullptr;
#else
		return false;
#endif // WITH_FREETYPE
	}

#if WITH_FREETYPE
	FORCEINLINE FT_Face GetFace() const
	{
		return FTFace;
	}
#endif // WITH_FREETYPE

	FORCEINLINE const TSet<FName>& GetAttributes() const
	{
		return Attributes;
	}

private:
	// Non-copyable
	FFreeTypeFace(const FFreeTypeFace&);
	FFreeTypeFace& operator=(const FFreeTypeFace&);

#if WITH_FREETYPE
	FT_Face FTFace;
	TArray<uint8> Memory;
#endif // WITH_FREETYPE

	TSet<FName> Attributes;
};


/**
 * Provides low-level glyph caching to avoid repeated calls to FT_Load_Glyph (see FCachedGlyphData).
 * Most of the data cached here is required for HarfBuzz, however a couple of things (such as the baseline and max character height) 
 * are used directly by the Slate font cache. Feel free to add more cached data if required, but please keep it in native FreeType 
 * format where possible - the goal here is to avoid calls to FT_Load_Glyph, *not* to perform data transformation to what Slate needs.
 */
class FFreeTypeGlyphCache
{
public:
#if WITH_FREETYPE
	struct FCachedGlyphData
	{
		FT_Short Height;
		FT_Glyph_Metrics GlyphMetrics;
		FT_Size_Metrics SizeMetrics; 
		TArray<FT_Vector> OutlinePoints;
	};

	bool FindOrCache(FT_Face InFace, const uint32 InGlyphIndex, const int32 InLoadFlags, const int32 InFontSize, const float InFontScale, FCachedGlyphData& OutCachedGlyphData);
#endif // WITH_FREETYPE

	void FlushCache();

private:
#if WITH_FREETYPE
	struct FCachedGlyphKey
	{
	public:
		FCachedGlyphKey(FT_Face InFace, const uint32 InGlyphIndex, const int32 InLoadFlags, const int32 InFontSize, const float InFontScale)
			: Face(InFace)
			, GlyphIndex(InGlyphIndex)
			, LoadFlags(InLoadFlags)
			, FontSize(InFontSize)
			, FontScale(InFontScale)
			, KeyHash(0)
		{
			KeyHash = HashCombine(KeyHash, GetTypeHash(Face));
			KeyHash = HashCombine(KeyHash, GetTypeHash(GlyphIndex));
			KeyHash = HashCombine(KeyHash, GetTypeHash(LoadFlags));
			KeyHash = HashCombine(KeyHash, GetTypeHash(FontSize));
			KeyHash = HashCombine(KeyHash, GetTypeHash(FontScale));
		}

		FORCEINLINE bool operator==(const FCachedGlyphKey& Other) const
		{
			return Face == Other.Face 
				&& GlyphIndex == Other.GlyphIndex
				&& LoadFlags == Other.LoadFlags
				&& FontSize == Other.FontSize
				&& FontScale == Other.FontScale;
		}

		FORCEINLINE bool operator!=(const FCachedGlyphKey& Other) const
		{
			return !(*this == Other);
		}

		friend inline uint32 GetTypeHash(const FCachedGlyphKey& Key)
		{
			return Key.KeyHash;
		}

	private:
		FT_Face Face;
		uint32 GlyphIndex;
		int32 LoadFlags;
		int32 FontSize;
		float FontScale;
		uint32 KeyHash;
	};

	TMap<FCachedGlyphKey, FCachedGlyphData> CachedGlyphDataMap;
#endif // WITH_FREETYPE
};


/**
 * Provides low-level advance caching to avoid repeated calls to FT_Get_Advance.
 */
class FFreeTypeAdvanceCache
{
public:
#if WITH_FREETYPE
	bool FindOrCache(FT_Face InFace, const uint32 InGlyphIndex, const int32 InLoadFlags, const int32 InFontSize, const float InFontScale, FT_Fixed& OutCachedAdvance);
#endif // WITH_FREETYPE

	void FlushCache();

private:
#if WITH_FREETYPE
	struct FCachedAdvanceKey
	{
	public:
		FCachedAdvanceKey(FT_Face InFace, const uint32 InGlyphIndex, const int32 InLoadFlags, const int32 InFontSize, const float InFontScale)
			: Face(InFace)
			, GlyphIndex(InGlyphIndex)
			, LoadFlags(InLoadFlags)
			, FontSize(InFontSize)
			, FontScale(InFontScale)
			, KeyHash(0)
		{
			KeyHash = HashCombine(KeyHash, GetTypeHash(Face));
			KeyHash = HashCombine(KeyHash, GetTypeHash(GlyphIndex));
			KeyHash = HashCombine(KeyHash, GetTypeHash(LoadFlags));
			KeyHash = HashCombine(KeyHash, GetTypeHash(FontSize));
			KeyHash = HashCombine(KeyHash, GetTypeHash(FontScale));
		}

		FORCEINLINE bool operator==(const FCachedAdvanceKey& Other) const
		{
			return Face == Other.Face 
				&& GlyphIndex == Other.GlyphIndex
				&& LoadFlags == Other.LoadFlags
				&& FontSize == Other.FontSize
				&& FontScale == Other.FontScale;
		}

		FORCEINLINE bool operator!=(const FCachedAdvanceKey& Other) const
		{
			return !(*this == Other);
		}

		friend inline uint32 GetTypeHash(const FCachedAdvanceKey& Key)
		{
			return Key.KeyHash;
		}

	private:
		FT_Face Face;
		uint32 GlyphIndex;
		int32 LoadFlags;
		int32 FontSize;
		float FontScale;
		uint32 KeyHash;
	};

	TMap<FCachedAdvanceKey, FT_Fixed> CachedAdvanceMap;
#endif // WITH_FREETYPE
};


/**
 * Provides low-level kerning-pair caching to avoid repeated calls to FT_Get_Kerning.
 */
class FFreeTypeKerningPairCache
{
public:
	struct FKerningPair
	{
		FKerningPair(const uint32 InFirstGlyphIndex, const uint32 InSecondGlyphIndex)
			: FirstGlyphIndex(InFirstGlyphIndex)
			, SecondGlyphIndex(InSecondGlyphIndex)
		{
		}

		FORCEINLINE bool operator==(const FKerningPair& Other) const
		{
			return FirstGlyphIndex == Other.FirstGlyphIndex 
				&& SecondGlyphIndex == Other.SecondGlyphIndex;
		}

		FORCEINLINE bool operator!=(const FKerningPair& Other) const
		{
			return !(*this == Other);
		}

		friend inline uint32 GetTypeHash(const FKerningPair& Key)
		{
			uint32 KeyHash = 0;
			KeyHash = HashCombine(KeyHash, Key.FirstGlyphIndex);
			KeyHash = HashCombine(KeyHash, Key.SecondGlyphIndex);
			return KeyHash;
		}

		uint32 FirstGlyphIndex;
		uint32 SecondGlyphIndex;
	};

#if WITH_FREETYPE
	bool FindOrCache(FT_Face InFace, const FKerningPair& InKerningPair, const int32 InKerningFlags, const int32 InFontSize, const float InFontScale, FT_Vector& OutKerning);
#endif // WITH_FREETYPE

	void FlushCache();

private:
#if WITH_FREETYPE
	struct FCachedKerningPairKey
	{
	public:
		FCachedKerningPairKey(FT_Face InFace, const FKerningPair& InKerningPair, const int32 InKerningFlags, const int32 InFontSize, const float InFontScale)
			: Face(InFace)
			, KerningPair(InKerningPair)
			, KerningFlags(InKerningFlags)
			, FontSize(InFontSize)
			, FontScale(InFontScale)
			, KeyHash(0)
		{
			KeyHash = HashCombine(KeyHash, GetTypeHash(Face));
			KeyHash = HashCombine(KeyHash, GetTypeHash(KerningPair));
			KeyHash = HashCombine(KeyHash, GetTypeHash(KerningFlags));
			KeyHash = HashCombine(KeyHash, GetTypeHash(FontSize));
			KeyHash = HashCombine(KeyHash, GetTypeHash(FontScale));
		}

		FORCEINLINE bool operator==(const FCachedKerningPairKey& Other) const
		{
			return Face == Other.Face 
				&& KerningPair == Other.KerningPair
				&& KerningFlags == Other.KerningFlags
				&& FontSize == Other.FontSize
				&& FontScale == Other.FontScale;
		}

		FORCEINLINE bool operator!=(const FCachedKerningPairKey& Other) const
		{
			return !(*this == Other);
		}

		friend inline uint32 GetTypeHash(const FCachedKerningPairKey& Key)
		{
			return Key.KeyHash;
		}

	private:
		FT_Face Face;
		FKerningPair KerningPair;
		int32 KerningFlags;
		int32 FontSize;
		float FontScale;
		uint32 KeyHash;
	};

	TMap<FCachedKerningPairKey, FT_Vector> CachedKerningPairMap;
#endif // WITH_FREETYPE
};
