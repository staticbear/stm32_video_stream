#include "viewform.h"
#include "control.h"
#include <QApplication>
#include <QPushButton>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringList>
#include <QStringListModel>
#include <QListView>
#include <QFile>

#define FRAME_WIDTH  1280
#define FRAME_HEIGHT 1024

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
 
ViewForm::ViewForm(QWidget *parent) : QWidget(parent)
{
	// main window
	setFixedSize(1024, 560);
	setWindowTitle("IP Camera control");

	// label for image
	lbl_image = new QLabel("test", this);
	lbl_image->setGeometry(10, 10, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	// ip list
	QStringList List;
    List << "device_ip::::::::::::device_id:::::::client_ip";
	model = new QStringListModel(this);
	model->setStringList(List);
	device_list = new QListView(this);
	device_list->setGeometry(10 + SCREEN_WIDTH + 10, 10, 280, 270);
    device_list->setModel(model);
	
	device_list_cnt = 0;
	flag_eth_init = 0;
	flag_device_connected = 0;
	flag_stream = 0;

	// list device button
	btn_listdevice = new QPushButton("List", this);
	btn_listdevice->setGeometry(10 + SCREEN_WIDTH + 10, 10 + 270 + 10, 110, 30);
	
	//edit ip input
	ipEdit = new QLineEdit(this);
	ipEdit->setInputMask("000.000.000.000;_");
	ipEdit->setGeometry(10 + SCREEN_WIDTH + 10 + 110 + 10, 10 + 270 + 10, 110, 30);
	
	// change ip button
	btn_chg_ip = new QPushButton("Change IP", this);
	btn_chg_ip->setGeometry(10 + SCREEN_WIDTH + 10, 10 + 270 + 10 + 30 + 10, 110, 30);
	
	//edit device id input
	devidEdit = new QLineEdit(this);
	devidEdit->setInputMask("00000000;_");
	devidEdit->setGeometry(10 + SCREEN_WIDTH + 10 + 110 + 10, 10 + 270 + 10 + 30 + 10, 110, 30);
	
	// Connect button
	btn_connect = new QPushButton("Connect", this);
	btn_connect->setGeometry(10 + SCREEN_WIDTH + 10, 10 + 270 + 10 + 30 + 10 + 30 + 10, 110, 30);
	
	// stream button
	btn_st_sp = new QPushButton("StartStream", this);
	btn_st_sp->setGeometry(10 + SCREEN_WIDTH + 10, 10 + 270 + 10 + 30 + 10 + 30 + 10 + 30 + 10, 110, 30);
	
	// take screen button
	btn_tk_screen = new QPushButton("TakeScreen", this);
	btn_tk_screen->setGeometry(10 + SCREEN_WIDTH + 10, 10 + 270 + 10 + 30 + 10 + 30 + 10 + 30 + 10 + 30 + 10, 110, 30);
	
	m_image = new QImage(FRAME_WIDTH, FRAME_HEIGHT, QImage::Format_RGB16);
	m_pixmap = QPixmap(FRAME_WIDTH, FRAME_HEIGHT);
	m_pixmap.fill(QColor(31,12,0));
	lbl_image->setPixmap(m_pixmap);
	
	//screen refresh timer
	timer = new QTimer();
	timer->setSingleShot(true);

	// NEW : Make the connection
	connect(device_list, SIGNAL(clicked(const QModelIndex)),this, SLOT(slotDeviceListItemClicked(QModelIndex)));
	connect(btn_listdevice, SIGNAL(pressed()), this, SLOT(slotBtnDeviceListPressed()));
	connect(btn_chg_ip, SIGNAL(pressed()), this, SLOT(slotBtnChangeIpPressed()));
	connect(btn_connect, SIGNAL(pressed()), this, SLOT(slotBtnConnectPressed()));
	connect(btn_st_sp, SIGNAL(pressed()), this, SLOT(slotBtnStreamPressed()));
	connect(btn_tk_screen, SIGNAL(pressed()), this, SLOT(slotBtnTakeScreenPressed()));
	connect(timer, SIGNAL(timeout()), this, SLOT(timerScreenRefreshSlot()));
	
	return;
 }
 
 //----------------------------------------------------------------------------------------------
 
 void ViewForm::GetDeviceList()
 {
	int ret;
	int rcv_size;
	QMessageBox msgBox;
	
	if(flag_device_connected)
	{
		msgBox.setText("Can't request device list while device connected");
		msgBox.exec();
		return;
	}
	
	CMD_ARG_T get_devices_buf[DEVICE_LIST_MAX];
	memset(get_devices_buf, 0, sizeof(get_devices_buf));
	ret = Eth_DeviceList(0x1771, (uint8_t *)get_devices_buf, sizeof(get_devices_buf), &rcv_size);
	if(ret != 0){
		
		msgBox.setText("Eth_DeviceList error");
		msgBox.exec();
		printf("Eth_DeviceList return %d\n", ret);
		return;
	}
	
	device_list_cnt = 0;
	for(int i = 0; i < (int)(rcv_size / sizeof(CMD_ARG_T)); i++){
		if(get_devices_buf[i].data[0] == ANSW_OK){
			memcpy(&device_list_array[device_list_cnt++], &get_devices_buf[i].data[1], sizeof(STATUS_PAR_T));
		}
	}
	
	QStringList List;
	QString str;
	List << "device_ip::::::::::::device_id:::::::client_ip";
	for(int i = 0; i < device_list_cnt; i++)
	{
		List << str.number(device_list_array[i].device_ip[0] & 0xFF) + "." + str.number(device_list_array[i].device_ip[0] >> 8 & 0xFF) + 
		       "." + str.number(device_list_array[i].device_ip[1] & 0xFF) + "." + str.number(device_list_array[i].device_ip[1] >> 8 & 0xFF) + 
		       "     " + str.number(device_list_array[i].device_id, 16) + 
		       "     " + str.number(device_list_array[i].client_ip[0] & 0xFF) + "." + str.number(device_list_array[i].client_ip[0] >> 8 & 0xFF) + 
		       "." + str.number(device_list_array[i].client_ip[1] & 0xFF) + "." + str.number(device_list_array[i].client_ip[1] >> 8 & 0xFF);
	}
	
	model->setStringList(List);
    device_list->setModel(model);
	return;
 }
 
//----------------------------------------------------------------------------------------------
 
void ViewForm::ChangeIP()
{
	QMessageBox msgBox;
	bool ok;
	QString str_new_ip = ipEdit->text();
	QString str_devid = devidEdit->text();
	QRegExp rx("[.]");
	QStringList list = str_new_ip.split(rx, QString::SkipEmptyParts);
	uint32_t new_ip = 0;
	
	for(int i = 0; i < 4; i++)
	{
		uint8_t tmp = (uint8_t)list.at(i).toUInt(&ok, 10); 
		if(ok == false){
			msgBox.setText("ip wrong format" + str_new_ip);
			msgBox.exec();
			return;
		}
		new_ip |= (tmp << (i * 8));
	}
	
	//printf("new_ip = %x \n", new_ip);
	
	uint32_t devid = str_devid.toUInt(&ok, 16); 
	if(ok == false){
		msgBox.setText("device id wrong format");
		msgBox.exec();
		return;
	}
	
	int ret = Eth_ChangeIp(0x1771, devid, new_ip);
	if(ret != 0){
		msgBox.setText("Eth_ChangeIp error");
		msgBox.exec();
	}

	return;
}

//---------------------------------------------------------------------------------------------- 

void ViewForm::TakeScreen()
{
	QMessageBox msgBox;
	QString fileName("screen.png");
	QFile file(fileName);
	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		m_pixmap.save(&file, "PNG");
	}
	else {
		msgBox.setText("can't save screen.png");
		msgBox.exec();
	}
	return;
}

//----------------------------------------------------------------------------------------------
 
void ViewForm::Connect()
{
	int ret;
	QMessageBox msgBox;
	if(flag_device_connected){
		msgBox.setText("Already connected");
		msgBox.exec();
		return;
	}
	
	if(flag_eth_init == 0){
		ret = Eth_Init();
		if(ret != 0){
			msgBox.setText("Already connected");
			msgBox.exec();
			printf("Eth_Init return %d\n", ret);
			return;
		}
		
		flag_eth_init = 1;
	}
	
	QString str = ipEdit->text();
	unsigned char pass[8] = {'r', 'o', 'o', 't', 'r', 'o', 'o', 't'}; 
	printf("test %s \n", str.toUtf8().constData());
	ret = Eth_Connect(str.toUtf8().constData(), 0x1771, &pass);
	if(ret != 0){
		msgBox.setText("can't connect to " + str);
		msgBox.exec();
		printf("Eth_Connect return %d\n", ret);
	}
	
	flag_device_connected = 1;
	
	return;
 }
 
 //----------------------------------------------------------------------------------------------
 
void ViewForm::StartStream()
{
	QMessageBox msgBox;
	int ret;
	
	if(flag_device_connected == 0)
	{
		msgBox.setText("device not connected");
		msgBox.exec();
		return;
	}
	
	if(flag_stream == 1){
		msgBox.setText("stream already started");
		msgBox.exec();
		return;
	}
	
	ret = Eth_CameraI2C_Write(0x12, 03);
	if(ret != 0){
		msgBox.setText("Eth_CameraI2C_Write reg 0x12 error");
		msgBox.exec();
		return;
	}
	
	ret = Eth_CameraI2C_Write(0x40, 0x10);
	if(ret != 0){
		msgBox.setText("Eth_CameraI2C_Write reg 0x40 error");
		msgBox.exec();
		return;
	}
	
	ret = Eth_StartStream(1280);
	if(ret != 0){
		msgBox.setText("Eth_StartStream error");
		msgBox.exec();
		return;
	}
	
	flag_stream = 1;
	
	return;
}
 
 //----------------------------------------------------------------------------------------------
 
void ViewForm::StopStream()
{
	QMessageBox msgBox;
	int ret;
	
	if(flag_device_connected == 0)
	{
		msgBox.setText("device not connected");
		msgBox.exec();
		return;
	}
	
	if(flag_stream == 0){
		msgBox.setText("stream already stopped");
		msgBox.exec();
		return;
	}
	
	ret = Eth_StopStream();
	if(ret != 0)
	{
		Eth_Disconnect();
		flag_device_connected = 0;
		btn_connect->setText("Disconnect");
		msgBox.setText("Stop stream error, device disconnected");
		msgBox.exec();
		return;
	}
	
	flag_stream = 0;
	
	return;
}
 
//----------------------------------------------------------------------------------------------
 
void ViewForm::Disconnect()
{
	QMessageBox msgBox;
	if(flag_device_connected == 0){
		msgBox.setText("not connected");
		msgBox.exec();
		return;
	}
	
	Eth_Disconnect();
	flag_device_connected = 0;
	
	return;
}

//----------------------------------------------------------------------------------------------

void ViewForm::makePixmap(uint16_t *pData, int width, int height)
{
	uint16_t *pSrc = 0, *pDst = 0;
	pSrc = pData;
	
	for(int y = 0; y < height; y++)
	{
		pDst = (uint16_t *)m_image->scanLine(y);
		for(int x = 0; x < width; x++)
			pDst[x] = *(pSrc++);
	}
	
	QImage zi;
	zi = m_image->scaled(SCREEN_WIDTH, SCREEN_HEIGHT, Qt::KeepAspectRatio);
	m_pixmap = QPixmap::fromImage(zi);
	lbl_image->setPixmap(m_pixmap);
	
	return;
}

//----------------------------------------------------------------------------------------------
 
ViewForm::~ViewForm()
{
	if(flag_device_connected)
		Disconnect();
		
	if(flag_eth_init)
		Eth_Free();
 
   return; 
}

//----------------------------------------------------------------------------------------------
 
void ViewForm::slotDeviceListItemClicked(QModelIndex index)
{
	int idx = index.row();
	
	//skip title
	if(idx == 0)
		return;
	else
		idx -= 1;
	
	QString str;
	
	str = str.number(device_list_array[idx].device_ip[0] & 0xFF) + "." + str.number(device_list_array[idx].device_ip[0] >> 8 & 0xFF) + 
		  "." + str.number(device_list_array[idx].device_ip[1] & 0xFF) + "." + str.number(device_list_array[idx].device_ip[1] >> 8 & 0xFF);
	
	ipEdit->setText(str);
	
	
	str = str.number(device_list_array[idx].device_id, 16);
	devidEdit->setText(str);
	
	return;
}

//----------------------------------------------------------------------------------------------
 
void ViewForm::slotBtnDeviceListPressed()
{
	GetDeviceList();
	return;
}

//----------------------------------------------------------------------------------------------

void ViewForm::slotBtnChangeIpPressed()
{
	ChangeIP();
	return;
}

//----------------------------------------------------------------------------------------------
 
void ViewForm::slotBtnConnectPressed()
{
	if(flag_device_connected)
		Disconnect();
	else
		Connect();
	
	if(flag_device_connected)
		btn_connect->setText("Disconnect");
	else
		btn_connect->setText("Connect");
	
	return;
}

//----------------------------------------------------------------------------------------------

void ViewForm::slotBtnStreamPressed()
{
	if(flag_stream == 0)
		StartStream();
	else
		StopStream();
	
	if(flag_stream == 1){
		timer->start(50);
		btn_st_sp->setText("StopStream");
	}
	else
		btn_st_sp->setText("StartStream");
	
	return;
}

//----------------------------------------------------------------------------------------------

void ViewForm::timerScreenRefreshSlot()
{
	int rdy_frame_number;
	uint16_t *ptrRdyframe;
	if(flag_stream == 0)
		return;
	
	rdy_frame_number = Eth_GetReadyFrameNumber();
	if(rdy_frame_number != -1){
		ptrRdyframe = (uint16_t *)GetFrame(rdy_frame_number);
		if(ptrRdyframe != 0)
			makePixmap(ptrRdyframe, FRAME_WIDTH, FRAME_HEIGHT);
		
		Eth_FrameFree(rdy_frame_number);
	}
	
	timer->start(50);
	
	return;
}

//----------------------------------------------------------------------------------------------

void ViewForm::slotBtnTakeScreenPressed()
{
	TakeScreen();
	return;
}
