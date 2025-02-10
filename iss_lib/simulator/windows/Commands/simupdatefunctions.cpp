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
#include "simupdatefunctions.h"

int updatelanes(QVector<QRadioButton*> radiobuttons,
    QVector<QProgressBar*> progressbars,
    long clock,
    QVector<long> clockvalues,
    QVector<double> progresstotal) {
    int lastcycles = 0;
    int showlastcycles = 0;
    bool shownlast = false;
    int median = 1;
    for (
        int i = 0; i < radiobuttons.size() - 1;
        i++) {  //loop who finds a numer in the text of each radiobutton and setting lastcylces to this value
        if (radiobuttons[i]->isChecked()) {
            QRegularExpression rx("[0-9]+");
            QRegularExpressionMatch match = rx.match(radiobuttons[i]->text());
            if (match.hasMatch()) {
                lastcycles = match.captured(0).toInt();
            }
        }
    }
    for (int i = 0; i < progresstotal.size(); i++) {
        progresstotal[i] = 0;
    }
    int pos = 0;
    for (int i = 0; i < progresstotal.size(); i++) {
        progresstotal[i] =
            progresstotal[i] /
            median;  //divides trough the number of calculations done to get the median
    }
    auto bar = progressbars.begin();
    for (int t = 0; t < progresstotal.size(); t++) {  //setting the value of the progressbars
        (*bar)->setValue(progresstotal[t]);
        progresstotal[t] = 0;
        bar++;
        if (bar == progressbars.end()) break;
    }

    return showlastcycles;
}

int updatelanestotal(QVector<QProgressBar*> progressbars, long clock) {
    int showlastcycles = 0;
    showlastcycles = clock;
    auto bar = progressbars.begin();
    return showlastcycles;
}
