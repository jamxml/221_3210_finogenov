#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <QByteArray>

class CryptoUtils {
public:
    int encrypt(const QByteArray &in, QByteArray &out, const QByteArray &key, const QByteArray &iv);
    int decrypt(const QByteArray &in, QByteArray &out, const QByteArray &key, const QByteArray &iv);
};

#endif // CRYPTO_UTILS_H
