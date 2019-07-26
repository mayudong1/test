#ifndef __AMF_H__
#define __AMF_H__

typedef enum
{ 
	AMF_NUMBER = 0, AMF_BOOLEAN, AMF_STRING, AMF_OBJECT,
	AMF_MOVIECLIP,		/* reserved, not used */
	AMF_NULL, AMF_UNDEFINED, AMF_REFERENCE, AMF_ECMA_ARRAY, AMF_OBJECT_END,
	AMF_STRICT_ARRAY, AMF_DATE, AMF_LONG_STRING, AMF_UNSUPPORTED,
	AMF_RECORDSET,		/* reserved, not used */
	AMF_XML_DOC, AMF_TYPED_OBJECT,
	AMF_AVMPLUS,		/* switch to AMF3 */
	AMF_INVALID = 0xff
} AMFDataType;

char* encode_int16(char* output, short value)
{
	output[0] = value >> 8;
	output[1] = value & 0xff;
	return output + 2;
}

char* encode_int24(char* output, int value)
{
	output[0] = value >> 16;
	output[1] = (value >> 8) & 0xff;
	output[2] = value & 0xff;
	return output + 3;
}

char* encode_int32(char* output, int value)
{
	output[0] = value >> 24;
	output[1] = (value >> 16) & 0xff;
	output[2] = (value >> 8) & 0xff;
	output[3] = value & 0xff;
	return output + 4;
}

char* encode_string(char* output, char* str)
{
	int len = strlen(str);
	if(len < 65535)
	{
		*output++ = AMF_STRING;	
		output = encode_int16(output, len);
	}
	else
	{
		*output++ = AMF_LONG_STRING;
		output = encode_int32(output, len);
	}
	memcpy(output, str, len);
	output += len;
	return output;
}

//need fix
char* encode_number(char* output, double value)
{ 
	*output++ = AMF_NUMBER;
	// memcpy(output, &value, 8);
	unsigned char *ci, *co;
    ci = (unsigned char *)&value;
    co = (unsigned char *)output;
    co[0] = ci[7];
    co[1] = ci[6];
    co[2] = ci[5];
    co[3] = ci[4];
    co[4] = ci[3];
    co[5] = ci[2];
    co[6] = ci[1];
    co[7] = ci[0];
	return output+8;
}

char* encode_bool(char* output, int value)
{
	*output++ = AMF_BOOLEAN;
	*output++ = value ? 1 : 0;
	return output;
}


char* encode_named_string(char* output, char* name, char* value)
{
	int name_len = strlen(name);
	output = encode_int16(output, name_len);
	memcpy(output, name, name_len);
	output += name_len;
	return encode_string(output, value);
}

char* encode_named_number(char* output, char* name, double value)
{
	int name_len = strlen(name);
	output = encode_int16(output, name_len);
	memcpy(output, name, name_len);
	output += name_len;
	return encode_number(output, value);
}

char* encode_named_bool(char* output, char* name, int value)
{
	int name_len = strlen(name);
	output = encode_int16(output, name_len);
	memcpy(output, name, name_len);
	output += name_len;
	return encode_bool(output, value);
}

int encode_int32_le(char* output, int value)
{
	output[0] = value;
    value >>= 8;
    output[1] = value;
    value >>= 8;
    output[2] = value;
    value >>= 8;
    output[3] = value;
    return 4;
}

int encode_int32_be(char* output, int value)
{
	output[0] = (value >> 24);
	output[1] = (value >> 16) & 0x00ff;
	output[2] = (value >> 8) & 0x0000ff;
	output[3] = value & 0x000000ff;
    return 4;
}

int encode_int24_be(char* output, int value)
{	
	output[0] = (value >> 16) & 0x00ff;
	output[1] = (value >> 8) & 0x0000ff;
	output[2] = value & 0x000000ff;
    return 3;
}


#endif



