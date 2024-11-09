#include "crypto_utils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <QDebug>
#include <vector>

int CryptoUtils::encrypt(const QByteArray &in, QByteArray &out, const QByteArray &key, const QByteArray &iv) {
    if (key.size() != 32 || iv.size() != 16) {
        qDebug() << "Неверная длина ключа или IV";
        return 1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qDebug() << "Не удалось создать контекст шифрования";
        return 1;
    }

    std::vector<unsigned char> encBuffer(in.size() + EVP_MAX_BLOCK_LENGTH);
    int len;
    int finalLen;

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.data()), reinterpret_cast<const unsigned char *>(iv.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        qDebug() << "Ошибка инициализации шифрования";
        return 1;
    }

    if (!EVP_EncryptUpdate(ctx, encBuffer.data(), &len, reinterpret_cast<const unsigned char *>(in.data()), in.size())) {
        EVP_CIPHER_CTX_free(ctx);
        qDebug() << "Ошибка обновления шифрования";
        return 1;
    }
    out.append(reinterpret_cast<char *>(encBuffer.data()), len);

    if (!EVP_EncryptFinal_ex(ctx, encBuffer.data() + len, &finalLen)) {
        EVP_CIPHER_CTX_free(ctx);
        qDebug() << "Ошибка финализации шифрования";
        return 1;
    }
    out.append(reinterpret_cast<char *>(encBuffer.data() + len), finalLen);

    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

int CryptoUtils::decrypt(const QByteArray &in, QByteArray &out, const QByteArray &key, const QByteArray &iv) {
    if (key.size() != 32 || iv.size() != 16) {
        qDebug() << "Неверная длина ключа или IV";
        return 1; // Неверная длина ключа или IV
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qDebug() << "Не удалось создать контекст расшифрования";
        return 1;
    }

    std::vector<unsigned char> decBuffer(in.size() + EVP_MAX_BLOCK_LENGTH);
    int len;
    int finalLen;

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.data()), reinterpret_cast<const unsigned char *>(iv.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        qDebug() << "Ошибка инициализации расшифрования";
        return 1;
    }

    if (!EVP_DecryptUpdate(ctx, decBuffer.data(), &len, reinterpret_cast<const unsigned char *>(in.data()), in.size())) {
        EVP_CIPHER_CTX_free(ctx);
        qDebug() << "Ошибка обновления расшифрования";
        return 1;
    }
    out.append(reinterpret_cast<char *>(decBuffer.data()), len);

    if (!EVP_DecryptFinal_ex(ctx, decBuffer.data() + len, &finalLen)) {
        EVP_CIPHER_CTX_free(ctx);
        qDebug() << "Ошибка финализации расшифрования";
        return 1;
    }
    out.append(reinterpret_cast<char *>(decBuffer.data() + len), finalLen);

    EVP_CIPHER_CTX_free(ctx);
    return 0;
}
