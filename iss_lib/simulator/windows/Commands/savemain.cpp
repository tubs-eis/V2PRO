/*
 *  * Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
 *                    Technische Universitaet Braunschweig, Germany
 *                    www.tu-braunschweig.de/en/eis
 * 
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * 
 */
#include "savemain.h"

void savemaintofile(int i,
    CommandWindow* Widget,
    QVector<QLineEdit*> offsets,
    QVector<QLineEdit*> sizes,
    bool* paused) {
    int offset, size;

    if (!*paused) {
        emit Widget->pauseSim();
    }

    offset = offsets[i]->text().toInt();
    size = sizes[i]->text().toInt();
    if ((offset + size) > (1024 * 1024 * 1024)) {
        QMessageBox warning;
        warning.setText("Typed area extends mainmemory please type again");
        warning.exec();
    } else if (offsets[i]->text().isEmpty() || sizes[i]->text().isEmpty()) {
        QMessageBox warning;
        warning.setText("Please type in offset and size");
        warning.exec();
    } else {
        QString filename = QFileDialog::getSaveFileName(
            Widget, QObject::tr("Save Data of Mainmemory"), "", QObject::tr("All Files (*)"));
        if (filename.isEmpty())
            return;
        else {
            QFile outputfile(filename);
            if (outputfile.open(QFile::WriteOnly | QFile::Truncate)) {
                for (int i = 0; i < size; i += 2) {
                    char buffer[2];
                    buffer[1] = Widget->main_memory[offset + i + 0];  // convert endianess
                    buffer[0] = Widget->main_memory[offset + i + 1];
                    ;
                    outputfile.write(buffer, 2);
                }
                outputfile.close();
            } else {
                QMessageBox::information(
                    Widget, QObject::tr("Unable to open file"), outputfile.errorString());
                return;
            }
        }
    }
}
