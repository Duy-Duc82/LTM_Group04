#include "FileDialogHelper.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

FileDialogHelper::FileDialogHelper(QObject *parent)
    : QObject(parent)
{
}

QString FileDialogHelper::openImageFile()
{
    QStringList filters;
    filters << "Image files (*.png *.jpg *.jpeg *.bmp *.gif)"
            << "PNG files (*.png)"
            << "JPEG files (*.jpg *.jpeg)"
            << "BMP files (*.bmp)"
            << "GIF files (*.gif)"
            << "All files (*.*)";

    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picturesPath.isEmpty()) {
        picturesPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "Chọn ảnh đại diện",
        picturesPath,
        filters.join(";;")
    );

    if (!fileName.isEmpty()) {
        emit fileSelected(fileName);
    }

    return fileName;
}

