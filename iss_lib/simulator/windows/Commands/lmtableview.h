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
#ifndef LOCALMEMORYTABLE_H
#define LOCALMEMORYTABLE_H

#include <QAbstractTableModel>
#include <QDebug>
#include <QLabel>
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

class LmTableModel : public QAbstractTableModel {
    Q_OBJECT
   public:
    LmTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool setData(const QModelIndex& index,
        const QVariant& value,
        int role = Qt::EditRole) override;  // for editing
    int basevalue = 10;
    int* base = &basevalue;
    void setLocalMemory(uint8_t* ref);

    static const int COLS = 4;
    static const int ROWS = 8192 / COLS;

   private:
    uint8_t* lm;
    bool gotLmRef;
    //    QString m_gridData[ROWS][COLS];  //holds text entered into QTableView

   signals:
    void editCompleted(const QString&);
};

namespace Ui {
class LmTableView;
}

class LmTableView : public QTableView {
    Q_OBJECT
   public:
    explicit LmTableView(QWidget* parent = nullptr);
    ~LmTableView();
    LmTableModel* modelData;
    int COLW;

   public slots:
    void setLocalMemory(int unit, uint8_t* ref);
    void selectLocalMemory(int unit);

   private:
    QVector<uint8_t*> lm_ref;
    int unit;
};

#endif  // LOCALMEMORYTABLE_H
