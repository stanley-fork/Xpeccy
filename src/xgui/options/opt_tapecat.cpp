#include "opt_tapecat.h"
#include "xcore/xcore.h"

//#include <QDebug>

// model

xTapeCatModel::xTapeCatModel(QObject* p):xTableModel(p) {
	rcnt = 0;
	inf = NULL;
}

int xTapeCatModel::rowCount(const QModelIndex&) const {
	return rcnt;
}

int xTapeCatModel::columnCount(const QModelIndex&) const {
	return 5;	// 6
}

void xTapeCatModel::fill(Tape* tap) {
	if (rcnt < tap->blkCount) {
		beginInsertRows(QModelIndex(), 0, tap->blkCount - rcnt - 1);
		endInsertRows();
	} else if (rcnt > tap->blkCount) {
		beginRemoveRows(QModelIndex(), 0, rcnt - tap->blkCount - 1);
		endRemoveRows();
	}
	rcnt = tap->blkCount;
	rcur = tap->block;
	// int fsz = rcnt * sizeof(TapeBlockInfo);
	if (inf) delete(inf);
	inf = new TapeBlockInfo[rcnt]; // (TapeBlockInfo*)realloc(inf, fsz);
	if (rcnt == 0) {
		inf = NULL;
	} else {
		switch(conf.prof.cur->zx->hw->grp) {
			case HWG_ZX:
				tapGetBlocksInfo(tap, inf, TFRM_ZX);
				break;
			case HWG_BK:
				tapGetBlocksInfo(tap, inf, TFRM_BK);
				break;
			default:
				for (int i = 0; i < rcnt; i++) {
					inf[i].type = -1;
					inf[i].name[0] = 0;
					inf[i].size = 0;
					inf[i].time = 0;
					inf[i].curtime = 0;
					inf[i].breakPoint = 0;
				}
				break;
		}
	}
	update();
}

static QVariant tcmName[5] = {"Brk","Dur","Time","Bytes","Name"};

QVariant xTapeCatModel::headerData(int sec, Qt::Orientation ori, int role) const {
	QVariant res;
	if (ori != Qt::Horizontal) return res;
	if (role != Qt::DisplayRole) return res;
	if ((sec < 0) || (sec >= columnCount())) return res;
	res = tcmName[sec];
	return res;
}

QVariant xTapeCatModel::data(const QModelIndex& idx, int role) const {
	QVariant res;
	if (!idx.isValid()) return res;
	int row = idx.row();
	int col = idx.column();
	if ((row < 0) || (row >= rowCount())) return res;
	if ((col < 0) || (col >= columnCount())) return res;
	if (inf == NULL) return res;
	//QFont fnt;
	switch (role) {
		case Qt::CheckStateRole:
			switch(col) {
				case 0: if (inf[row].breakPoint)
						res = Qt::Checked;
					break;
			}
			break;
		case X_BackgroundRole:
			if (row == rcur) res = QColor(Qt::darkGray);
			break;
		case Qt::ForegroundRole:
			if (row == rcur) res = QColor(Qt::white);
			break;
		//case Qt::FontRole:
			//fnt.setBold(row == rcur);
			//res = fnt;
			//break;
		case Qt::DisplayRole:
			switch(col) {
				case 1: res = QString(getTimeString(inf[row].time).c_str());
					break;
				case 2: if (row == rcur)
						res = QString(getTimeString(inf[row].curtime).c_str());
					break;
				case 3:	if (inf[row].size > 0)
						res = inf[row].size;
					break;
				case 4: res = QString::fromLocal8Bit(inf[row].name);
					break;
			}
			break;
	}
	return  res;
}

// table

xTapeCatTable::xTapeCatTable(QWidget* p):QTableView(p) {
	model = new xTapeCatModel();
	setModel(model);
}

// tape player window will reset scroll on update
void xTapeCatTable::fill(Tape* tape) {
	model->fill(tape);
	scrollTo(model->index(tape->block, 0), QAbstractItemView::EnsureVisible);
	setEnabled(tape->blkCount > 0);
}
