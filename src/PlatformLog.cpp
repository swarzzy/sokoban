#include "PlatformLog.h"
#include <stdarg.h>
#include <string.h>

namespace AB
{
	
	u32 DateTimeToString(DateTime* dt, char* buffer, u32 bufferSize)
	{
		if (dt->hour < 24 && dt->minute < 60 && dt->seconds < 60)
		{
			if (bufferSize >= DATETIME_STRING_SIZE)
			{
				i32 written = FormatString(buffer, bufferSize,
										   "%02u16:%02u16:%02u16",
										   dt->hour, dt->minute, dt->seconds);
				return DATETIME_STRING_SIZE - 1;
			}
		}
		else
		{
			if (bufferSize >= DATETIME_STRING_SIZE)
			{
				FormatString(buffer, bufferSize, "00:00:00");
				return DATETIME_STRING_SIZE - 1;
			}
		}
		return 1;
	}

	typedef void(OutCharFn)(char** at, u32* destSize, char ch);

	static void OutChar(char** at, u32* destSize, char ch)
	{
		if (*destSize)
		{
			(*destSize)--;
			**at = ch;
			*at = *at + 1;
		}
	}

	static void OutCharConsole(char** at, u32* destSize, char ch)
	{
		// @TODO: Batching!!!
		if (ch)
		{
			ConsolePrint((void*)(&ch), 1);
		}
	}

	u64 StringLength(const char* str)
	{
		u64 count = 0;
		while (*str)
		{
			count++;
			str++;
		}
		return count;
	}

	static i32 StringToInt32(const char* at, char** newAt)
	{
		i32 result = 0;
		b32 negative = false;
		if (*at == '-')
		{
			negative = true;
			at++;
		}
		while ((*at >= '0') && (*at <= '9'))
		{
			result *= 10;
			result += (*at - '0');
			at++;
		}
		
		*newAt = (char*)at;
		return negative ? result * -1 : result;
	}

// NOTE: Clang says that all args that is less than 32 bit will be promoted to 32 bit size.
// There is no such warnings on msvc. So just reading all args that are smaller than 32 bits as 32 bit integers

	static u64 ReadArgUnsignedInteger(u32 size, va_list* args)
	{
		u64 result = 0;
		switch (size)
		{
		case 8: {
			result = va_arg(*args, u32);
		} break;
		case 16: {
			result = va_arg(*args, u32);
		} break;
		case 32: {
			result = va_arg(*args, u32);
		} break;
		case 64: {
			result = va_arg(*args, u64);
		} break;
		default: {
			// TODO: Handle it more properly
			result = 0;
		} break;
		}
		return result;
	}

	static i64 ReadArgSignedInteger(u32 size, va_list* args)
	{
		i64 result = 0;
		switch (size)
		{
		case 8: {
			result = va_arg(*args, i32);
		} break;
		case 16: {
			result = va_arg(*args, i32);
		} break;
		case 32: {
			result = va_arg(*args, i32);
		} break;
		case 64: {
			result = va_arg(*args, i64);
		} break;
		default: {
			// TODO: Handle it more properly
			result = 0;
		} break;
		}
		return result;
	}

	static f64 ReadArgFloat(u32 size, va_list* args)
	{
		f64 result = 0;
		switch (size)
		{
		case 32: {
			// NOTE: Reading float32 as float64 instead of taking address 
			// and casting it to a float64* because floating point 
			// varargs actually implicitly cast to double
			result = (va_arg(*args, f64));
		} break;
		case 64: {
			result = va_arg(*args, f64);
		} break;
		default: {
			// TODO: Handle it more properly
			result = 0;
		} break;
		}
		return result;
	}

	static u32 Uint64ToASCII(char* buffer, u32 bufferSize, u64 value, u32 base,
							 const char* baseTable, OutCharFn* outFunc)
	{
		u32 written = 0;
		char* beg = buffer;
		if (value == 0)
		{
			outFunc(&buffer, &bufferSize, baseTable[0]);
			written++;
		}
		else
		{
			while (value != 0)
			{
				u64 digitIndex = value % base;
				char digit = baseTable[digitIndex];
				outFunc(&buffer, &bufferSize, digit);
				written++;
				value /= base;
			}
		}
		char* end = buffer - 1;
		while (beg < end)
		{
			char tmp = *beg;
			*beg = *end;
			*end = tmp;
			beg++;
			end--;
		}
		return written;
	}

	static u32 Float64ToASCII(char* buffer, u32 bufferSize, f64 value, u32 precision, u32 base,
							  const char* baseTable, OutCharFn* outFunc)
	{
		if (value < 0)
		{
			outFunc(&buffer, &bufferSize, '-');
			value = -value;
		}
		u32 written = 0;
		u64 intPart = (u64)value;
		f64 fractPart = value - (f64)intPart;
		written += Uint64ToASCII(buffer, bufferSize, intPart, base, baseTable, outFunc);
		buffer += written;
		bufferSize -= written;

		outFunc(&buffer, &bufferSize, '.');
		written++;

		for (u32 pIndex = 0; pIndex < precision; pIndex++)
		{
			fractPart *= (f64)base;
			u32 integer = (u32)fractPart;
			fractPart -= (f32)integer;
			outFunc(&buffer, &bufferSize, baseTable[integer]);
			written++;
		}
		return written;
	}

	struct FormatProperties
	{
		b32 forceSign;
		b32 padWithZeros;
		b32 leftJustify;
		b32 hidePositiveSign;
		b32 zeroAnnotation;
		b32 widthSpecified;
		u32 width;
		b32 precisionSpecified;
		u32 precision;
	};

	enum class OutputType
	{
		Unknown = 0,
		Uint,
		Int,
		UintHex,
		UintOct,
		Char,
		String,
		Float,
		FloatHex,
		Uintptr,
		PercentSign,
		QueryWrittenChars,
	};

	struct TypeProperties
	{
		OutputType type;
		u16 size;
	};

	static FormatProperties ParseFormat(const char** formatAt, va_list* args)
	{
		FormatProperties fmt = {};
		const char* at = *formatAt;
		//
		// Parsing flags
		//
		b32 parsing = true;
		while (parsing)
		{
			switch (*at)
			{
			case '+': { fmt.forceSign = true; } break;
			case '0': { fmt.padWithZeros = true; } break;
			case '-': { fmt.leftJustify = true;	} break;
			case ' ': { fmt.hidePositiveSign = true; } break;
			case '#': {	fmt.zeroAnnotation = true; } break;
			default: { parsing = false; } break;
			}
			if (parsing)
				at++;
		}
		//
		// Parsing width
		//
		if (*at == '*')
		{
			fmt.width = va_arg(*args, int);
			fmt.widthSpecified = true;
			at++;
		}
		else if ((*at >= '0') && (*at <= '9'))
		{
			char* newAt;
			fmt.width = StringToInt32(at, &newAt);
			at = newAt;
			fmt.widthSpecified = true;
		}
		//
		// Parse precision
		//
		if (*at == '.')
		{
			at++;
			if (*at == '*')
			{
				fmt.precision = va_arg(*args, int);
				fmt.precisionSpecified = true;
			}
			else if ((*at >= '0') && (*at <= '9'))
			{
				fmt.precisionSpecified = true;
				char* newAt;
				fmt.precision = StringToInt32(at, &newAt);
				at = newAt;
			}
		}
		*formatAt = at;
		return fmt;
	}

	static TypeProperties ParseType(const char** formatAt)
	{
		TypeProperties type = {};
		const char* at = *formatAt;

		if (at[0] == 'i' &&
			at[1] == '8')
		{
			type.type = OutputType::Int;
			type.size = 8;
			*formatAt = at + 2;
		}
		else if (at[0] == 'i' &&
		 		 at[1] == '1' &&
				 at[2] == '6')
		{
			type.type = OutputType::Int;
			type.size = 16;
			*formatAt = at + 3;
		}
		else if (at[0] == 'i' &&
				 at[1] == '3' &&
				 at[2] == '2')
		{
			type.type = OutputType::Int;
			type.size = 32;
			*formatAt = at + 3;
		}
		else if (at[0] == 'i' &&
				 at[1] == '6' &&
				 at[2] == '4')
		{
			type.type = OutputType::Int;
			type.size = 64;
			*formatAt = at + 3;
		}
		else if (at[0] == 'u' &&
				 at[1] == '8')
		{
			if (at[2] == 'h')
			{
				type.type = OutputType::UintHex;
				*formatAt = at + 3;
			}
			else if (at[2] == 'o')
			{
				type.type = OutputType::UintOct;
				*formatAt = at + 3;
			}
			else
			{
				type.type = OutputType::Uint;
				*formatAt = at + 2;
			}
			type.size = 8;
		}
		else if (at[0] == 'u' &&
			     at[1] == '1' &&
				 at[2] == '6')
		{
			if (at[3] == 'h')
			{
				type.type = OutputType::UintHex;
				*formatAt = at + 4;
			}
			else if (at[3] == 'o')
			{
				type.type = OutputType::UintOct;
				*formatAt = at + 4;
			}
			else
			{
				type.type = OutputType::Uint;
				*formatAt = at + 3;
			}
			type.size = 16;
		}
		else if (at[0] == 'u' &&
			     at[1] == '3' &&
				 at[2] == '2')
		{
			if (at[3] == 'h')
			{
				type.type = OutputType::UintHex;
				*formatAt = at + 4;
			}
			else if (at[3] == 'o')
			{
				type.type = OutputType::UintOct;
				*formatAt = at + 4;
			}
			else
			{
				type.type = OutputType::Uint;
				*formatAt = at + 3;
			}
			type.size = 32;
		}
		else if (at[0] == 'u' &&
				 at[1] == '6' &&
				 at[2] == '4')
		{
			if (at[3] == 'h')
			{
				type.type = OutputType::UintHex;
				*formatAt = at + 4;
			}
			else if (at[3] == 'o') {
				type.type = OutputType::UintOct;
				*formatAt = at + 4;
			}
			else {
				type.type = OutputType::Uint;
				*formatAt = at + 3;
			}
			type.size = 64;
		} else
			if (at[0] == 'c')
			{
				type.type = OutputType::Char;
				type.size = 0;
				*formatAt = at + 1;
			}
			else if (at[0] == 's')
			{
				type.type = OutputType::String;
				type.size = 0;
				*formatAt = at + 1;
			}
			else if (at[0] == 'p')
			{
				type.type = OutputType::Uintptr;
				type.size = 64;
				*formatAt = at + 1;
			}
			else if (at[0] == 'f' &&
					 at[1] == '3' &&
					 at[2] == '2')
			{
				if (at[3] == 'h')
				{
					type.type = OutputType::FloatHex;
					*formatAt = at + 4;
				}
				else
				{
					type.type = OutputType::Float;
					*formatAt = at + 3;
				}
				type.size = 32;
			}
			else if (at[0] == 'f' &&
					 at[1] == '6' &&
					 at[2] == '4')
			{
				if (at[3] == 'h')
				{
					type.type = OutputType::FloatHex;
					*formatAt = at + 4;
				}
				else
				{
					type.type = OutputType::Float;
					*formatAt = at + 3;
				}
				type.size = 64;
			}
			else if (at[0] == '%')
			{
				type.type = OutputType::PercentSign;
				*formatAt = at + 1;
				type.size = 0;
			}
			else if (at[0] == 'n')
			{
				type.type = OutputType::QueryWrittenChars;
				*formatAt = at + 1;
				type.size = 0;
			}
			else
			{
				type.type = OutputType::Unknown;
				type.size = 0;
				// NOTE: Error: Unknown type
			}
		return type;
	}

	static void PrintInteger(const TypeProperties* type, const FormatProperties* fmt,
							 char** outputBufferAt, u32* outputBufferFree, OutCharFn* outFunc, va_list* args)
	{
		const char decimalTable[] = "0123456789";
		const char upperHexTable[] = "0123456789ABCDEF";
		const char lowerHexTable[] = "0123456789abcdef";
		u32 defaultFloatPrecision = 5;

		const u32 tempBufferSize = 64;
		char tempBuffer[tempBufferSize];

		char* tempBufferBegin = tempBuffer;
		u32 tempBufferFree = 64;
		char* tempBufferAt = tempBuffer;

		char prefix[2] = { 0 };

		switch (type->type)
		{
		case OutputType::Int: {
			i64 val = ReadArgSignedInteger(type->size, args);
			b32 negative = val < 0;
			if (negative)
			{
				val = -val;				
			}
			u32 written = Uint64ToASCII(tempBufferAt, tempBufferSize,
										(u64)val, 10, decimalTable, OutChar);
			tempBufferAt += written;
			if (negative)
			{
				prefix[0] = '-';
			}
			else if (fmt->forceSign && !fmt->hidePositiveSign)
			{
				prefix[0] = '+';
			}
			else if (fmt->hidePositiveSign)
			{
				prefix[0] = ' ';
			}
		} break;
		case OutputType::Uint: {
			u64 val = ReadArgUnsignedInteger(type->size, args);
			u32 written = Uint64ToASCII(tempBufferAt, tempBufferSize,
										(u64)val, 10, decimalTable, OutChar);
			tempBufferAt += written;
			if (fmt->forceSign && !fmt->hidePositiveSign)
			{
				prefix[0] = '+';
			}
			else if (fmt->hidePositiveSign)
			{
				prefix[0] = ' ';
			}
		} break;
		case OutputType::UintOct: {
			u64 val = ReadArgUnsignedInteger(type->size, args);
			u32 written = Uint64ToASCII(tempBufferAt, tempBufferSize,
										(u64)val, 8, decimalTable, OutChar);
			tempBufferAt += written;
			if (fmt->zeroAnnotation && (val != 0))
			{
				// TODO: should it be like below in hex
				//OutChar(&tempBufferAt, &tempBufferFree, '0');
				if (fmt->zeroAnnotation && (val != 0))
				{
					prefix[0] = '0';
				}
			}
		} break;
		case OutputType::UintHex:
		{
			u64 val = ReadArgUnsignedInteger(type->size, args);
			u32 written = Uint64ToASCII(tempBufferAt, tempBufferSize,
										(u64)val, 16, lowerHexTable, OutChar);
			tempBufferAt += written;
			if (fmt->zeroAnnotation && (val != 0))
			{
				prefix[0] = '0';
				prefix[1] = 'x';
			}
		} break;
		case OutputType::Uintptr: {
			void* val = va_arg(*args, void*);
			u32 written = Uint64ToASCII(tempBufferAt, tempBufferSize,
										*(uptr*)&val, 16, lowerHexTable, OutChar);
			tempBufferAt += written;

		} break;
		default: {
		} break;
		}

		u32 prefixLength = prefix[0] ? 1 : 0 + prefix[1] ? 1 : 0;
		u32 usePrecision = fmt->precision;
		if (!fmt->precisionSpecified)
		{
			usePrecision = (u32)(tempBufferAt - tempBufferBegin);
		}
		b32 leftJustify = fmt->padWithZeros ? false : fmt->leftJustify;

		u32 useWidth = fmt->width;
		if (!fmt->widthSpecified)
		{
			useWidth = usePrecision + prefixLength;
		}

		// Prefix
		if (fmt->padWithZeros)
		{
			if (prefix[0] && useWidth)
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[0]);
				useWidth--;
			}
			if (prefix[1])
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[1]);
				useWidth--;
			}

			if (!leftJustify)
			{
				while (useWidth > usePrecision)
				{
					outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
					useWidth--;
				}
			}
		}
		else
		{
			if (!leftJustify)
			{
				while (useWidth > (usePrecision + prefixLength))
				{
					outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
					useWidth--;
				}
			}

			if (prefix[0] && useWidth)
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[0]);
				useWidth--;
			}
			if (prefix[1])
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[1]);
				useWidth--;
			}
		}

		if (usePrecision > useWidth)
		{
			usePrecision = useWidth;
		}

		while ((u32)(tempBufferAt - tempBufferBegin) < usePrecision)
		{
			outFunc(outputBufferAt, outputBufferFree, '0');
			usePrecision--;
			useWidth--;
		}

		char* tbufPtr = tempBufferBegin;
		while ((tempBufferAt != tbufPtr) && usePrecision)
		{
			outFunc(outputBufferAt, outputBufferFree, *(tbufPtr));
			tbufPtr++;
			usePrecision--;
			useWidth--;
		}

		if (leftJustify)
		{
			while (useWidth)
			{
				outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
				useWidth--;
			}
		}

	}

	static void PrintChars(const TypeProperties* type, const FormatProperties* fmt,
						   char** outputBufferAt, u32* outputBufferFree, OutCharFn* outFunc, va_list* args)
	{
		if (type->type == OutputType::String)
		{
			char nullBuff[7] = "(null)";
			char* str = va_arg(*args, char*);
			char* begin = str;
			char* at = str;
			if (str) {
				u32 counter = 0;
				at += StringLength(str);
			}
			else
			{
				begin = nullBuff;
				at = nullBuff + 6;
			}

			u32 usePrecision = fmt->precision;
			if (!fmt->precisionSpecified)
			{
				usePrecision = (u32)(at - begin);
			}

			b32 leftJustify = fmt->padWithZeros ? false : fmt->leftJustify;

			u32 useWidth = fmt->width;
			if (!fmt->widthSpecified)
			{
				useWidth = usePrecision;
			}

			if (!leftJustify)
			{
				while (useWidth > usePrecision)
				{
					outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
					useWidth--;
				}
			}

			if (usePrecision > useWidth)
			{
				usePrecision = useWidth;
			}

			while ((u32)(at - begin) < usePrecision)
			{
				outFunc(outputBufferAt, outputBufferFree, '0');
				usePrecision--;
				useWidth--;
			}

			char* tbufPtr = begin;
			while ((at != tbufPtr) && usePrecision)
			{
				outFunc(outputBufferAt, outputBufferFree, *(tbufPtr));
				tbufPtr++;
				usePrecision--;
				useWidth--;
			}

			if (leftJustify)
			{
				while (useWidth)
				{
					outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
					useWidth--;
				}
			}
		}
		else if (type->type == OutputType::PercentSign)
		{
			outFunc(outputBufferAt, outputBufferFree, '%');
		}
		else if (type->type == OutputType::Char)
		{
			// NOTE: Chars are converted to int in varargs
			char val = va_arg(*args, int);
			outFunc(outputBufferAt, outputBufferFree, val);
		}
		else
		{
			// TODO: Error unknown type
		}

	}

	static u32 CountUint64Digits(u64 value, u32 base)
	{
		if (value == 0)
		{
			return 1;
		}

		u32 count = 0;
		while (value != 0)
		{
			value /= base;
			count++;
		}

		return count;
	}

	static void PrintFloat(const TypeProperties* type, const FormatProperties* fmt,
 						   char** outputBufferAt, u32* outputBufferFree,
						   OutCharFn* outFunc, va_list* args)
	{
		const u32 tempBufferSize = 64;
		char tempBuffer[tempBufferSize];
		char* tempBufferBegin = tempBuffer;
		u32 tempFree = 64;
		char* tempAt = tempBuffer;
		char prefix[2] = { 0 };

		u32 precision = 5;

		switch (type->type)
		{
		case OutputType::Float:
		{
			if (fmt->precisionSpecified)
			{
				precision = fmt->precision;
			}
			// NOTE: increment precision because of the dot '.'
			precision += 1;
			f64 val = ReadArgFloat(type->size, args);
			u32 written = Float64ToASCII(tempAt, tempBufferSize, (f64)val, precision, 10, "0123456789", OutChar);
			u32 intDigits = CountUint64Digits((u64)(val >= 0 ? val : -val), 10);
			precision += intDigits;
			tempAt += written;

		} break;
#if 0 // Not supported for now
		case OutputType::FloatHex:
		{
			if (fmt->precisionSpecified)
			{
				precision = fmt->precision;
			}
			// NOTE: increment precision because of the dot '.'
			precision += 1;
			f64 val = ReadArgFloat(type->size, &args);
			u32 written = Float64ToASCII(tempAt, tempBufferSize, (f64)val, precision, 16, "0123456789abcdef");
			u32 intDigits = CountUint64Digits((u64)val, 16);
			precision += intDigits;
			tempAt += written;
		} break;
#endif
		default:
		{
			// TODO: Error. Unsupported type
		} break;
		}

		u32 prefixLength = prefix[0] ? 1 : 0 + prefix[1] ? 1 : 0;
		u32 usePrecision = precision;

		b32 leftJustify = fmt->padWithZeros ? false : fmt->padWithZeros;

		u32 useWidth = fmt->width;
		if (!fmt->widthSpecified)
		{
			useWidth = usePrecision + prefixLength;
		}

		if (fmt->padWithZeros)
		{

			if (prefix[0] && useWidth)
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[0]);
				useWidth--;
			}
			if (prefix[1])
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[1]);
				useWidth--;
			}

			if (!leftJustify)
			{
				while (useWidth > usePrecision)
				{
					outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
					useWidth--;
				}
			}
		}
		else
		{
			if (leftJustify)
			{
				while (useWidth > (usePrecision + prefixLength))
				{
					outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
					useWidth--;
				}
			}

			if (prefix[0] && useWidth)
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[0]);
				useWidth--;
			}
			if (prefix[1])
			{
				outFunc(outputBufferAt, outputBufferFree, prefix[1]);
				useWidth--;
			}
		}

		if (usePrecision > useWidth)
		{
			usePrecision = useWidth;
		}

		while ((u32)(tempAt - tempBufferBegin) < usePrecision)
		{
			outFunc(outputBufferAt, outputBufferFree, '0');
			usePrecision--;
			useWidth--;
		}

		char* tbufPtr = tempBufferBegin;
		while ((tempAt != tbufPtr) && usePrecision)
		{
			outFunc(outputBufferAt, outputBufferFree, *(tbufPtr));
			tbufPtr++;
			usePrecision--;
			useWidth--;
		}

		if (leftJustify)
		{
			while (useWidth)
			{
				outFunc(outputBufferAt, outputBufferFree, fmt->padWithZeros ? '0' : ' ');
				useWidth--;
			}
		}
	}

	static i32 FormatStringV(char* buffer, u32 bufferSize, const char* format, OutCharFn* outFunc, va_list* args)
	{
		int result = -1;
		u32 destSize = bufferSize;
		char* destAt = buffer;
		if (destSize)
		{
			const char* at = format;
			while (at[0] && destSize > 0)
			{
				if (at[0] == '%')
				{
					at++;
					FormatProperties fmt = ParseFormat(&at, args);
					TypeProperties type = ParseType(&at);

					if (type.type != OutputType::Unknown)
					{

						if (type.type == OutputType::Int     ||
							type.type == OutputType::Uint    ||
							type.type == OutputType::UintOct ||
							type.type == OutputType::UintHex ||
							type.type == OutputType::Uintptr)
						{
							PrintInteger(&type, &fmt, &destAt, &destSize, outFunc, args);
						}
						else if (type.type == OutputType::Char   ||
							  	 type.type == OutputType::String ||
								 type.type == OutputType::PercentSign)
						{
							PrintChars(&type, &fmt, &destAt, &destSize, outFunc, args);
						}
						else if (type.type == OutputType::Float ||
								 type.type == OutputType::FloatHex)
						{
							PrintFloat(&type, &fmt, &destAt, &destSize, outFunc, args);
						}
						else if (type.type == OutputType::QueryWrittenChars)
						{
							u32* ptr = va_arg(*args, u32*);
							*ptr = (u32)(destAt - buffer);
						}
						else
						{
							// TODO: Error Unsupported type
						}
					}

				}
				else
				{
					outFunc(&destAt, &destSize, at[0]);
					at++;
				}
			}

			result = destSize ? bufferSize - destSize : -1;

			if (destSize)
			{
				outFunc(&destAt, &destSize, '\0');
			}
			else
			{
				//*(destAt - 1) = '\0';
				destAt -= 1;
				destSize += 1;
				outFunc(&destAt, &destSize, '\0');
			}
		}

		return result;
	}

	i32 FormatString(char* buffer, u32 bufferSize, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		u32 result = FormatStringV(buffer, bufferSize, format, OutChar, &args);
		va_end(args);
		return result;
	}

	// NOTE: OutCharConsole outputs characters one by one.
	// This is might be extremely slow.
	// FormatStringV that takes nullptr look like dirty hack 
	// and might be actually unsafe
	// Fast solution is just make two of these functions:
	// first takes buffer, second just prints to console.
	void PrintString(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		FormatStringV(nullptr, 1, format, OutCharConsole, &args);
		va_end(args);
	}

	i32 StringToInt32(const char* at)
	{
		i32 result = 0;
		b32 negative = false;
		if (*at == '-')
		{
			negative = true;
			at++;
		}
		while ((*at >= '0') && (*at <= '9'))
		{
			result *= 10;
			result += (*at - '0');
			at++;
		}
		return negative ? result * -1 : result;
	}

	u32 ToString(char* buffer, u32 bufferSize, u64 value)
	{
		return Uint64ToASCII(buffer, bufferSize, value, 10, "0123456789", OutChar);
	}

	u32 ToString(char* buffer, u32 bufferSize, f64 value, u32 precision)
	{
		return Float64ToASCII(buffer, bufferSize, value, precision, 10, "0123456789", OutChar);
	}

	// LOG
	// TODO: Global variable here is bad!
	static LogLevel g_LogLevel = LOG_INFO;

	void Log(LogLevel level, const char* file, const char* func, u32 line, const char* fmt, ...)
	{
		if (level <= g_LogLevel)
		{
			va_list args;
			va_start(args, fmt);

			switch (level)
			{
			case LOG_INFO: {
				DateTime time = GetLocalTime();
				char timeBuffer[DATETIME_STRING_SIZE];
				DateTimeToString(&time, timeBuffer, DATETIME_STRING_SIZE);
				ConsoleSetColor(CONSOLE_COLOR_DARKGREEN, CONSOLE_COLOR_BLACK);
				PrintString("%s: ", timeBuffer);
				FormatStringV(nullptr, 1, fmt, OutCharConsole, &args);
				PrintString("\n\n");
				ConsoleSetColor(CONSOLE_COLOR_DARKWHITE, CONSOLE_COLOR_BLACK);
			} break;

			case LOG_WARN: {
				DateTime time = GetLocalTime();
				char timeBuffer[DATETIME_STRING_SIZE];
				DateTimeToString(&time, timeBuffer, DATETIME_STRING_SIZE);
				ConsoleSetColor(CONSOLE_COLOR_DARKYELLOW, CONSOLE_COLOR_BLACK);
				PrintString("%s: ", timeBuffer);
				FormatStringV(nullptr, 1, fmt, OutCharConsole, &args);
				char fileBuffer[256];
				memcpy(fileBuffer, file, 256);
				CutFilenameFromEnd(fileBuffer);
				PrintString("\n->FILE: %s\n->FUNC: %s\n->LINE: %u32\n\n", fileBuffer, func, line);
				ConsoleSetColor(CONSOLE_COLOR_DARKWHITE, CONSOLE_COLOR_BLACK);
			} break;

			case LOG_ERROR: {
				DateTime time = GetLocalTime();
				char timeBuffer[DATETIME_STRING_SIZE];
				DateTimeToString(&time, timeBuffer, DATETIME_STRING_SIZE);
				ConsoleSetColor(CONSOLE_COLOR_RED, CONSOLE_COLOR_BLACK);
				PrintString("%s: ", timeBuffer);
				FormatStringV(nullptr, 1, fmt, OutCharConsole, &args);
				char fileBuffer[256];
				memcpy(fileBuffer, file, 256);
				CutFilenameFromEnd(fileBuffer);
				PrintString("\n->FILE: %s\n->FUNC: %s\n->LINE: %u32\n\n", fileBuffer, func, line);
				ConsoleSetColor(CONSOLE_COLOR_DARKWHITE, CONSOLE_COLOR_BLACK);
			} break;

			case LOG_FATAL: {
				DateTime time = GetLocalTime();
				char timeBuffer[DATETIME_STRING_SIZE];
				DateTimeToString(&time, timeBuffer, DATETIME_STRING_SIZE);
				ConsoleSetColor(CONSOLE_COLOR_DARKRED, CONSOLE_COLOR_BLACK);
				PrintString("%s: ", timeBuffer);
				FormatStringV(nullptr, 1, fmt, OutCharConsole, &args);
				char fileBuffer[256];
				memcpy(fileBuffer, file, 256);
				CutFilenameFromEnd(fileBuffer);
				PrintString("\n->FILE: %s\n->FUNC: %s\n->LINE: %u32\n\n", fileBuffer, func, line);
				ConsoleSetColor(CONSOLE_COLOR_DARKWHITE, CONSOLE_COLOR_BLACK);
			} break;

			default: {
			} break;
			}
			va_end(args);
		}
	}

	void LogAssertV(LogLevel level, const char* file, const char* func, u32 line,
				   const char* assertStr, const char* fmt, va_list* args)
	{
		DateTime time = GetLocalTime();
		char timeBuffer[DATETIME_STRING_SIZE];
		DateTimeToString(&time ,timeBuffer, DATETIME_STRING_SIZE);
		ConsoleSetColor(CONSOLE_COLOR_DARKRED, CONSOLE_COLOR_BLACK);
		PrintString("%s: ASSERTION FAILED!\n", timeBuffer);
		
		if (fmt && args)
		{
			FormatStringV(nullptr, 1, fmt, OutCharConsole, args);
		}
		
		char fileBuffer[256];
		memcpy(fileBuffer, file, 256);
		CutFilenameFromEnd(fileBuffer);
		PrintString("\n->EXPR: %s \n->FILE: %s\n->FUNC: %s\n->LINE: %u32\n\n",assertStr, fileBuffer, func, line);
		ConsoleSetColor(CONSOLE_COLOR_DARKWHITE, CONSOLE_COLOR_BLACK);		
	}

	void LogAssert(LogLevel level, const char* file, const char* func, u32 line, const char* assertStr)
	{
		LogAssert(level, file, func, line, assertStr, nullptr);
	}

	void LogAssert(LogLevel level, const char* file, const char* func, u32 line,
				   const char* assertStr, const char* fmt, ...)
	{
		if (fmt)
		{
			va_list args;
			va_start(args, fmt);
			LogAssertV(level, file, func, line, assertStr, fmt, &args);
			va_end(args);
		}
		else
		{
			LogAssertV(level, file, func, line, assertStr, nullptr, nullptr);			
		}
	}

	void CutFilenameFromEnd(char* str, char separator)
	{
		u64 beg = 0;
		u64 end = strlen(str);
		bool found = false;

		for (u64 i = end; i > 0; i--)
		{
			if (str[i] == separator)
			{
				beg = i + 1;
				found = true;
				break;
			}
		}

		if (found)
		{
			u32 fill = 0;
			for (u64 i = beg; i < end; i++)
			{
				str[fill] = str[i];
				fill++;
			}
			str[end - beg] = '\0';
		}
	}

	GetFilenameFromPathRet  GetFilenameFromPath(const char* path, char* buffer, u32 buffer_size)
	{
#if defined(AB_PLATFORM_WINDOWS)
		char sep_1 = '\\';
		char sep_2 = '/';
#elif defined(AB_PLATFORM_LINUX)
		char sep_1 = '/';
		char sep_2 = '/';

#else
#error Unsupported OS
#endif
		GetFilenameFromPathRet result = {};
		u64 last_sep_pos = 0;
		u64 end = strlen(path);
		b32 has_sep = false;

		for (u64 i = end; i > 0; i--)
		{
			if (path[i] == sep_1 || path[i] == sep_2)
			{
				last_sep_pos = i;
				has_sep = true;
				break;
			}
		}

		u64 name_length = 0;
		if (has_sep)
		{
			name_length = end - last_sep_pos;
		}
		else
		{
			name_length = end;
		}

		if (name_length <= buffer_size)
		{
			for (u64 i = 0; i < name_length - 1; i++)
			{
				buffer[i] = path[last_sep_pos + i + 1];
				result.written++;
			}
			buffer[name_length - 1] = '\0';
			result.written++;
			result.succeeded = true;
		}
		else
		{
			result.written = name_length;
			result.succeeded = false;
		}
		return result;
	}
}
