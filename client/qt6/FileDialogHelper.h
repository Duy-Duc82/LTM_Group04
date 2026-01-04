#ifndef FILEDIALOGHELPER_H
#define FILEDIALOGHELPER_H

#include <QObject>
#include <QString>

class FileDialogHelper : public QObject
{
    Q_OBJECT

public:
    explicit FileDialogHelper(QObject *parent = nullptr);

    Q_INVOKABLE QString openImageFile();

signals:
    void fileSelected(const QString &filePath);
};

#endif // FILEDIALOGHELPER_H

