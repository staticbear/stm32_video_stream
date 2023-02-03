#ifndef VIEWFORM_H
#define VIEWFORM_H

#include "control.h"

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>
#include <QStringListModel>
#include <QListView>
#include <QTimer>

#define DEVICE_LIST_MAX 	8

class QPushButton;
class ViewForm : public QWidget
{
	Q_OBJECT	
public:
	explicit ViewForm(QWidget *parent = 0);
	~ViewForm();
	void makePixmap(uint16_t *pData, int width, int height);
	void GetDeviceList();
	void ChangeIP();
	void Connect();
	void Disconnect();
	void StartStream();
	void StopStream();
	void TakeScreen();
	
private:
	STATUS_PAR_T device_list_array[DEVICE_LIST_MAX];
	int device_list_cnt;
	int flag_eth_init;
	int flag_device_connected;
	int flag_stream;
	
	QImage *m_image;
	QPixmap m_pixmap;
	QStringList *strlist_device;
	QPushButton *btn_listdevice;
	QPushButton *btn_chg_ip;
	QPushButton *btn_connect;
	QPushButton *btn_st_sp;
	QPushButton *btn_tk_screen;
	QLabel *lbl_image;
	QLineEdit *ipEdit;	
	QLineEdit *devidEdit;
	QStringListModel *model;
	QListView *device_list;
	QTimer *timer;
private slots:
	void slotBtnConnectPressed();
	void slotBtnDeviceListPressed();
	void slotBtnChangeIpPressed();
	void slotBtnStreamPressed();
	void slotBtnTakeScreenPressed();
	void slotDeviceListItemClicked(QModelIndex index);
	void timerScreenRefreshSlot();
};

#endif // VIEWFORM_H