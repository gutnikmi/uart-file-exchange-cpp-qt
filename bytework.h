#include <stdio.h>
#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

const unsigned char DLE = 0x07;
const unsigned char STX = 0x02;
const unsigned char ETX = 0x03;
inline std::string crc8_res = "CRC8 not calculated";

unsigned char gencrc(unsigned char *data, size_t len);
int count_DLE(unsigned char *str, int size);
void dup(unsigned char * src, unsigned char * dst, int size);
void un_dup(unsigned char * src, unsigned char * dst, int size);
void pack_bytes(unsigned char * in, unsigned char * out, int size_b);
size_t convert_hex(uint8_t *dest, size_t count, const char *src);
std::string convert_to_string(const unsigned char* arr, size_t size);
std::string unpack_bytes(std::string in);
void writeToFile(const QString& str);
QString readFromFile();
