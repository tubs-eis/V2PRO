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
#include "savesettings.h"

void savesettings(
    QVector<QLineEdit*> offsets, QVector<QLineEdit*> sizes, QList<QCheckBox*> checkedtorestore) {
    QSettings settings;
    QDir dir;
    if (!dir.exists("~/.vprosim/")) {
        dir.mkdir("~/.vprosim/");
    }
    settings.setPath(QSettings::NativeFormat, QSettings::UserScope, "~/.vprosim/");
    settings.beginWriteArray("valuestorestore");
    for (int i = 0; i < 4; ++i) {
        settings.setArrayIndex(i);
        settings.setValue("offsetvalue", offsets[i]->text().toInt());
        settings.setValue("sizevalue", sizes[i]->text().toInt());
    }
    settings.endArray();
    QSettings checkedsettings;
    checkedsettings.setPath(QSettings::NativeFormat, QSettings::UserScope, "~/.vprosim/");

    checkedsettings.beginWriteArray("checkedtorestore");
    for (int i = 0; i < checkedtorestore.size(); i++) {
        checkedsettings.setArrayIndex(i);
        checkedsettings.setValue("checked", checkedtorestore[i]->isChecked());
    }
    checkedsettings.endArray();
}

void restoresettings(
    QVector<QLineEdit*> offsets, QVector<QLineEdit*> sizes, QList<QCheckBox*> checkedtorestore) {
    QSettings settings;
    QDir dir;
    if (!dir.exists("~/.vprosim/")) {
        dir.mkdir("~/.vprosim/");
    }
    settings.setPath(QSettings::NativeFormat, QSettings::UserScope, "~/.vprosim/");
    QSettings checkedsettings;
    checkedsettings.setPath(QSettings::NativeFormat, QSettings::UserScope, "~/.vprosim/");
    checkedsettings.beginReadArray("checkedtorestore");
    for (int i = 0; i < checkedtorestore.size(); i++) {
        checkedsettings.setArrayIndex(i);
        checkedtorestore[i]->setChecked(checkedsettings.value("checked").toBool());
    }
    checkedsettings.endArray();
    checkedsettings.beginReadArray("valuestorestore");
    for (int i = 0; i < 4; ++i) {
        checkedsettings.setArrayIndex(i);
        offsets[i]->setText(checkedsettings.value("offsetvalue").toString());
        sizes[i]->setText(checkedsettings.value("sizevalue").toString());
    }
}
