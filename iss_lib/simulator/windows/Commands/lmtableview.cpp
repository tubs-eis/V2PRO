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
#include "lmtableview.h"

LmTableView::LmTableView(QWidget* parent) : QTableView(parent), COLW(75) {
    modelData = new LmTableModel(this);
    this->setModel(modelData);
    unit = 0;
    for (int i = 0; i < modelData->COLS; i++) {
        this->setColumnWidth(i, COLW);
    }
}

LmTableView::~LmTableView() {}

void LmTableView::setLocalMemory(int unit, uint8_t* ref) {
    while (lm_ref.size() <= unit) {
        //        qWarning() << "[GUI.TableView] got new units reference... unit " << unit << ". Adding elements to ref vector...";
        lm_ref.append(nullptr);
    }

    lm_ref[unit] = ref;
    selectLocalMemory(unit);
    for (int i = 0; i < modelData->COLS; i++) {
        this->setColumnWidth(i, COLW);
    }
}

void LmTableView::selectLocalMemory(int unit) {
    if (lm_ref.size() <= unit) {
        //        qWarning() << "[GUI.TableView] Set LM to unit " << unit << " but ref vector only has " << lm_ref.size() << " elements!";
        return;
    }
    this->unit = unit;
    modelData->setLocalMemory(lm_ref[unit]);

    this->setModel(modelData);
    //this->resizeColumnsToContents();
    //this->resizeColumnsToContents();
    //this->resizeRowsToContents();
}

LmTableModel::LmTableModel(QObject* parent) : QAbstractTableModel(parent), gotLmRef(false) {}

int LmTableModel::rowCount(const QModelIndex& /*parent*/) const {
    return ROWS;
}
int LmTableModel::columnCount(const QModelIndex& /*parent*/) const {
    return COLS;
}
Qt::ItemFlags LmTableModel::flags(const QModelIndex& index) const {
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

void LmTableModel::setLocalMemory(uint8_t* ref) {
    this->lm = ref;
    this->gotLmRef = true;
    emit dataChanged(QModelIndex(), QModelIndex());
    this->submit();
}

QVariant LmTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QString::number(section);
    } else if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
        return QString::number(section * COLS);
    }
    return QVariant();
}

// managing data in the table (get from referenced data array)
QVariant LmTableModel::data(const QModelIndex& index, int role) const {
    int row = index.row();
    int col = index.column();
    uint32_t data2;
    switch (role) {
        case Qt::DisplayRole:  // show data text
            if (!gotLmRef) return QString("No Ref");
            data2 = uint32_t(lm[2 * (row * COLS + col)]) +
                    uint32_t(lm[2 * (row * COLS + col) + 1] << 8);
            if ((uint32_t(data2 >> 15u) & 1) == 1) data2 |= 0xFFFFF0000;
            if (basevalue == 2 and data2 != 0) {
                return QString::number(int(data2), basevalue).rightJustified(16, '0').right(16);
            } else if (basevalue == 16 and data2 != 0) {
                return QString::number(int(data2), basevalue).rightJustified(4, '0').right(4);
            } else {
                return QString::number(int(data2), basevalue);
            }
        case Qt::FontRole:
            //change font only for cell(0,0)
            //  QFont boldFont;
            //  boldFont.setBold(true);
            //  return boldFont;
            break;
        case Qt::BackgroundRole:
            //return QBrush(Qt::red); // e.g. change background if (row == 0 && col == 1)
            break;
        case Qt::TextAlignmentRole:
            //return Qt::AlignRight + Qt::AlignVCenter; // e.g. text alignment
            break;
        case Qt::CheckStateRole:
            // return Qt::Checked; //add a checkbox to cell
            break;
    }
    return QVariant();
}

bool LmTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == Qt::EditRole) {
        //if (!checkIndex(index))
        //    return false;

        if (!gotLmRef) return false;

        int row = index.row();
        int col = index.column();

        int val = value.toInt();

        lm[2 * (row * COLS + col) + 1] = uint8_t((val >> 8) & 0xff);
        lm[2 * (row * COLS + col)] = uint8_t(val & 0xff);

        emit editCompleted(value.toString());
        emit dataChanged(index, index);
        return true;
    }
    return false;
}
