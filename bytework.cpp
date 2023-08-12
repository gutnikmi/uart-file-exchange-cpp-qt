#include"bytework.h"
unsigned char gencrc(unsigned char *data, size_t len)
{
    unsigned char crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (unsigned char)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}


int count_DLE(unsigned char *str, int size)
{
    int dles = 0;
    for (int i = 0; i <size; i++)
    {
        if(str[i] == DLE)
            ++dles;
    };
    return dles;

}

void dup(unsigned char * src, unsigned char * dst, int size)
{
    int ofst = 0;
    for (int i = 0; i <size; i++)
    {
        if (src[i] == DLE)
        {
            dst[i+ofst] = src[i];
            dst[i+1+ofst] = DLE;
            ++ofst;
        }
        else
        {
            dst[i+ofst] = src[i];
        }
    };
}

void pack_bytes(unsigned char * in, unsigned char * out, int size_b)
{
    unsigned char temp_arr[size_b + count_DLE(in, size_b)];
    const unsigned char CRC8 = gencrc(in, size_b);
    dup(in, temp_arr, size_b);
    size_b = size_b + count_DLE(in, size_b);
    unsigned char byte_pack [size_b + 5];
    byte_pack [0] = DLE;
    byte_pack [1] = STX;
    memcpy(byte_pack + 2, temp_arr, size_b);
    byte_pack [size_b + 2] = CRC8;
    byte_pack [size_b + 3] = DLE;
    byte_pack [size_b + 4] = ETX;
    memcpy(out, byte_pack, size_b+5);
}

size_t convert_hex(uint8_t *dest, size_t count, const char *src) {
    char buf[3] = {0,0,0};
    size_t i = 0;
    for (i = 0; i < count && *src; i++) {
        buf[0] = *src++;
        buf[1] = '\0';
        if (*src) {
            buf[1] = *src++;
            buf[2] = '\0';
        }
        if (sscanf(buf, "%hhx", &dest[i]) != 1)
            break;
    }
    return i;
}

std::string convert_to_string(const unsigned char* arr, size_t size) {

    std::string result;
    for (int i = 0; i < size; i++) {
        char hex[3]  = {0,0,0};
        sprintf(hex, "%02x", arr[i]);
        result += hex;
    }
    return result;
}

void un_dup(unsigned char * src, unsigned char * dst, int size)
{
    for (int i = 2, j = 0; i <size-3; i++, j++)
    {
        if (src[i] == DLE)
        {
            dst[j] = src[i];
            ++i;
        }
        else
        {
            dst[j] = src[i];
        }
    };
}

std::string unpack_bytes(std::string in)
{
    int length = in.length();
    int arr_ln = length/2;
    unsigned char str_char [arr_ln];
    convert_hex(str_char, length, in.c_str());
    unsigned char str_crc8 = str_char[arr_ln - 3];
    int dles;
    if (count_DLE(str_char, arr_ln) - 2 == 0)
        dles = 0;
    else
        dles = (count_DLE(str_char, arr_ln) - 2) /2;
    int up_ln = (arr_ln - 5) - dles;
    unsigned char up_bytes [up_ln];
    un_dup(str_char, up_bytes, arr_ln);
    unsigned char bytes_crc8 = gencrc(up_bytes, up_ln);
    if (str_crc8 == bytes_crc8)
        crc8_res = "Crc matches";
    else
        crc8_res = "Crc matches";
    return convert_to_string(up_bytes, up_ln);
}

QString readFromFile()
{
    QString str;
    QFile file("in.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        str = QString::fromUtf8(file.readAll());
        file.close();
    }
    return str;
}

void writeToFile(const QString& str)
{
    QFile file("out.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        stream << str;
        file.close();
    }
}
