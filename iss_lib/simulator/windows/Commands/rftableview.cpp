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
#include "rftableview.h"

RfTableView::RfTableView(QWidget* parent) : QTableView(parent), COLW(75) {
    modelData = new RfTableModel(this);
    this->setModel(modelData);
    lane = 0;
    for (int i = 0; i < modelData->COLS; i++) {
        this->setColumnWidth(i, this->COLW);
    }
}

RfTableView::~RfTableView() {}

void RfTableView::setRegisterFile(int lane, uint8_t* ref) {
    while (rf_ref.size() <= lane) {
        //        qWarning() << "[GUI.TableView] got new units reference... unit " << lane << ". Adding elements to ref vector...";
        rf_ref.append(nullptr);
    }

    rf_ref[lane] = ref;
    selectRegisterFile(lane);
    for (int i = 0; i < modelData->COLS; i++) {
        this->setColumnWidth(i, this->COLW);
    }
}

void RfTableView::selectRegisterFile(int lane) {
    if (rf_ref.size() <= lane) {
        //        qWarning() << "[GUI.TableView] Set LM to unit " << lane << " but ref vector only has " << rf_ref.size() << " elements!";
        return;
    }
    this->lane = lane;
    modelData->setRegisterFile(rf_ref[lane]);

    this->setModel(modelData);
    //this->resizeColumnsToContents();
    //this->resizeRowsToContents();
}

RfTableModel::RfTableModel(QObject* parent) : QAbstractTableModel(parent), gotRfRef(false) {}

int RfTableModel::rowCount(const QModelIndex& /*parent*/) const {
    return ROWS;
}
int RfTableModel::columnCount(const QModelIndex& /*parent*/) const {
    return COLS;
}
Qt::ItemFlags RfTableModel::flags(const QModelIndex& index) const {
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

void RfTableModel::setRegisterFile(uint8_t* ref) {
    this->rf = ref;
    this->gotRfRef = true;
    emit dataChanged(QModelIndex(), QModelIndex());
    this->submit();
}

QVariant RfTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return QString::number(section);
    } else if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
        return QString::number(section * COLS);
    }
    return QVariant();
}

// managing data in the table (get from referenced data array)
QVariant RfTableModel::data(const QModelIndex& index, int role) const {
    int row = index.row();
    int col = index.column();
    uint32_t data2;

    switch (role) {
        case Qt::DisplayRole:  // show data text
            if (!gotRfRef) return QString("No Ref");

            data2 = ((uint32_t)(rf[3 * (row * COLS + col)]) +
                     (uint32_t)(rf[3 * (row * COLS + col) + 1] << 8) +
                     (uint32_t)(rf[3 * (row * COLS + col) + 2] << 16));
            if ((uint32_t(data2 >> 23u) & 1) == 1) data2 |= 0xFF000000;
            if (basevalue == 2 and data2 != 0) {
                return QString::number(int(data2), basevalue).rightJustified(24, '0').right(24);
            } else if (basevalue == 16 and data2 != 0) {
                return QString::number(int(data2), basevalue).rightJustified(6, '0').right(6);
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

bool RfTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == Qt::EditRole) {
        // if (!checkIndex(index))
        //   return false;

        if (!gotRfRef) return false;

        int row = index.row();
        int col = index.column();

        int val = value.toInt();

        rf[3 * (row * COLS + col) + 2] = uint8_t((val >> 16) & 0xff);
        rf[3 * (row * COLS + col) + 1] = uint8_t((val >> 8) & 0xff);
        rf[3 * (row * COLS + col)] = uint8_t(val & 0xff);

        emit editCompleted(value.toString());
        emit dataChanged(index, index);
        return true;
    }
    return false;
}
