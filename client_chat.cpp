/*
* - Client GUI App - 
- app does support nicknames
- send messages either hitting 'enter' key or clicking 'send' button
- client will get chat history from server
- sended message will be broadcasted to all clients through server
*/

#define QT_NO_DEPRECATED_WARNINGS

#include <QApplication>
#include <QWidget>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QInputDialog>
#include <QTcpSocket>

#include <fstream>
#include <iostream>

//-----------------
std::pair<std::string, unsigned short> host_ip_and_port(std::string file_name);
//-----------------
class Chat_Window : public QWidget
{
public:
	Chat_Window();
private:
	void on_connected();
	void send_message(); // just sending to server
	void on_readyRead(); // from server (including your own message), 
private:
	// ---database---
	QStandardItemModel message_data_model; //qobject
	QListView message_view; //qwidget
	//------
	QVBoxLayout base_V_layout;
	// --- input bar ---
	QWidget input_group;//qwidget
	QHBoxLayout input_H_layout;
	QLineEdit input_edit_field;
	QPushButton input_send_button;
	// --- connections ---
	QTcpSocket socket;
	QString nickname;
};
Chat_Window::Chat_Window()
	:message_data_model{ this },
	message_view{ this },
	base_V_layout{ this },
	input_group{ this },
	input_H_layout{&input_group },
	input_edit_field{ &input_group },
	input_send_button{"Send",&input_group }
{
	setWindowIcon(QIcon{ "images/chat_icon.png" });

	nickname = QInputDialog::getText(this, "Nickname", "Enter your nickname:");
	if (nickname.isEmpty())
		nickname = "Anonymous";

	resize(420, 600);

	base_V_layout.setContentsMargins(0, 0, 0, 0);
	base_V_layout.addWidget(&message_view);
	base_V_layout.addWidget(&input_group);

	message_view.setModel(&message_data_model);
	message_view.setEditTriggers(QAbstractItemView::NoEditTriggers);
	message_view.setSelectionMode(QAbstractItemView::NoSelection);
	message_view.setFocusPolicy(Qt::NoFocus);
	message_view.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	input_H_layout.setContentsMargins(8, 8, 8, 8);
	input_H_layout.addWidget(&input_edit_field);
	input_H_layout.addWidget(&input_send_button);

	input_edit_field.setPlaceholderText("Type a message...");

	connect(&input_send_button, &QPushButton::clicked, this, &Chat_Window::send_message);
	connect(&input_edit_field, &QLineEdit::returnPressed, this, &Chat_Window::send_message);

	connect(&socket, &QTcpSocket::connected, this, &Chat_Window::on_connected);
	connect(&socket, &QTcpSocket::readyRead, this, &Chat_Window::on_readyRead);

	auto ip_and_port{ host_ip_and_port("host_config.txt") };
	socket.connectToHost(ip_and_port.first.c_str(), ip_and_port.second);
}
void Chat_Window::send_message()
{
	const QString text = input_edit_field.text().trimmed();
	if (text.isEmpty())
		return;

	socket.write(text.toUtf8()+'\n');
	input_edit_field.clear();
}
void Chat_Window::on_connected()
{
	socket.write("NICK:" + nickname.toUtf8()+'\n');
	message_data_model.appendRow(new QStandardItem{ "[Connected to server]" });
}
void Chat_Window::on_readyRead()
{
	while (socket.canReadLine())
	{
		QByteArray line = socket.readLine().trimmed(); // trimmed() because 'enter' key would make double row
		message_data_model.appendRow(new QStandardItem{ QString::fromUtf8(line) });
		message_view.scrollToBottom();
	}
}
//-----------------
int main(int argc, char* argv[])
try 
{
	QApplication app{ argc, argv };
	Chat_Window w;
	w.show();
	return app.exec();
}
catch (const std::runtime_error& surprise)
{
	std::ofstream ofs{ "runtime_error.txt" };
	ofs << surprise.what();
	return 1;
}
catch (const std::exception& surprise)
{
	std::ofstream ofs{ "exception.txt" };
	ofs << surprise.what();
	return 2;
}
catch (...)
{
	std::ofstream ofs{ "unknown_exception.txt" };
	ofs << "Caught an unknown exception.";
	return 3;
}
//-----------------
std::pair<std::string, unsigned short> host_ip_and_port(std::string file_name)
{
	std::ifstream ifs{ file_name };
	if (!ifs) throw std::runtime_error{ "No " + file_name + " file found." };
	std::string ip;
	unsigned short port{};
	ifs >> ip >> port;
	return { ip, port };
}