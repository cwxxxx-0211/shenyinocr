#ifndef CRYPTOUTILS_H
#define CRYPTOUTILS_H


#include <QCryptographicHash>
#include <QByteArray>
#include <QString>

class CryptoUtils {
public:
    static QString encrypt(const QString &data, const QString &key) {
        QByteArray byteData = data.toUtf8();
        QByteArray byteKey = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
        QByteArray encrypted;
        for (int i = 0; i < byteData.size(); ++i) {
            encrypted.append(byteData[i] ^ byteKey[i % byteKey.size()]);
        }
        return QString(encrypted.toBase64());
    }

    static QString decrypt(const QString &data, const QString &key) {
        QByteArray byteData = QByteArray::fromBase64(data.toUtf8());
        QByteArray byteKey = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
        QByteArray decrypted;
        for (int i = 0; i < byteData.size(); ++i) {
            decrypted.append(byteData[i] ^ byteKey[i % byteKey.size()]);
        }
        return QString(decrypted);
    }
};
#endif // CRYPTO_UTILS_H
