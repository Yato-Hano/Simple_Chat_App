/*
- Server Console App -
- every recieved message will be broadcasted to all clients
- broadcasts greatings to new clients as well
- does not broadcast a message on a client disconnect
- sends messages history to every new client
*/

#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QDebug>

#include <print>
#include <iostream>
#include <fstream>

// property names
inline const char* const has_greeted{ "has_greeted" };
inline const char* const nickname{ "nickname" };
//-------------
unsigned short host_port(std::string file_name);
//-------------
class Server : public QObject
{
public:
	Server();
	~Server();
private:
	void on_newConnection();
	void on_client_readyRead();
	void on_client_disconnected();
private:
	QTcpServer tcp_server;
	QList<QTcpSocket*>clients;
	QList<QByteArray> message_history;
};
Server::Server()
{
	connect(&tcp_server,&QTcpServer::newConnection,this,&Server::on_newConnection);

	auto port = host_port("host_config.txt");
	if (!tcp_server.listen(QHostAddress::Any, port)) {
		qDebug() << "Failed to start server:" << tcp_server.errorString();
		return;
	}
	qDebug() << "Server listening on port" << port;
}
Server::~Server()
{
	for (QTcpSocket* client : clients)
	{
		client->write("Server shut down");
		client->waitForBytesWritten(1000);
		client->disconnectFromHost();
		delete client;
	}
}
void Server::on_newConnection()
{
	QTcpSocket* client = tcp_server.nextPendingConnection();
	clients.append(client);
	client->setProperty(nickname, QByteArray{"Anonymous"});
	client->setProperty(has_greeted , false);

	connect(client, &QTcpSocket::readyRead, this, &Server::on_client_readyRead);
	connect(client, &QTcpSocket::disconnected, this, &Server::on_client_disconnected);

	qDebug() << "Client connected:" << client->peerAddress().toString();
}
void broadcast(const QByteArray& message,const QList<QTcpSocket*>& clients)
{
	for (QTcpSocket* client : clients)
	{
		client->write(message);
	}
}
void greet_new_user(QTcpSocket* sender, const QByteArray& line, const QList<QTcpSocket*>& clients, QList<QByteArray>& history)
{
	sender->setProperty(has_greeted, true);
	if (line.startsWith("NICK:"))
	{
		QByteArray new_user_nickname{ line.mid(5) };
		sender->setProperty(nickname, new_user_nickname);
		
		// send messages history to newuser
		for (const QByteArray& old_message : history)
		{
			sender->write(old_message);
		}
		
		QByteArray welcome_message{ "Welcome: " + new_user_nickname + '\n' };
		broadcast(welcome_message, clients);
		history.append(welcome_message);
	}
}
void Server::on_client_readyRead()
{
	QTcpSocket* sender = qobject_cast<QTcpSocket*>(QObject::sender());
	if (!sender)
		return;

	while (sender->canReadLine())
	{
		QByteArray received_line = sender->readLine().trimmed();
		qDebug() << "Received from[" << sender->property(nickname).toByteArray() << "]:" << received_line;

		if (!sender->property(has_greeted).toBool())
		{
			greet_new_user(sender, received_line,clients,message_history); // broadcasts
			continue;
		}
		QByteArray new_message{ sender->property(nickname).toByteArray() + ": " + received_line + '\n' };
		broadcast(new_message, clients);

		if (message_history.size() > 500)
			message_history.removeFirst();

		message_history.append(new_message);
	}
}
void Server::on_client_disconnected()
{
	QTcpSocket* sender = qobject_cast<QTcpSocket*>(QObject::sender());
	if (!sender)
		return;

	clients.removeOne(sender);
	sender->deleteLater();
	qDebug() << "Client ["<< sender->property(nickname).toString()<<"] disconnected";
}
//-----------------
int main(int argc, char* argv[])
try {

	QCoreApplication app(argc, argv);
	Server server;
	return app.exec();
	return 0;
}
catch (const std::runtime_error& surprise)
{
	std::println(std::cerr, "Runtime error: {}.", surprise.what());
	std::system("pause");
	return 1;
}
catch (const std::exception& surprise)
{
	std::println(std::cerr, "Exception: {}.", surprise.what());
	std::system("pause");
	return 2;
}
catch (...)
{
	std::println(std::cerr, "Caught an unknown exception.");
	std::system("pause");
	return 3;
}
//-----------------
unsigned short host_port(std::string file_name)
{
	std::ifstream ifs{ file_name };
	if (!ifs) throw std::runtime_error{ "No " + file_name + " file found." };
	std::string ip;
	unsigned short port{};
	ifs >> ip >> port;
	return port;
}