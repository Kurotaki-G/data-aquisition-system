#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>


#include <iostream>
#include <fstream>

#include <vector>

#include <ctime>
#include <iomanip>
#include <sstream>

std::time_t string_to_time_t(const std::string& time_string) {
    std::tm tm = {};
    std::istringstream ss(time_string);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::mktime(&tm);
}
std::string time_t_to_string(std::time_t time) {
    std::tm* tm = std::localtime(&time);
    std::ostringstream ss;
    ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::vector<std::string> split_string(std::string str, const char* op) {
	std::vector<std::string> result;
	std::string rest = str, block;
	size_t operator_position = rest.find_first_of(op);
	while (operator_position != std::string::npos) {
		block = rest.substr(0, operator_position);
		rest = rest.substr(operator_position + 1);
		operator_position = rest.find_first_of(op);
		result.push_back(block);
	}
	
	if (rest.length() > 0)
		result.push_back(rest);
		
	return result;
}

int write_in_file(std::string msg_sensor_id, std::string msg_time, std::string msg_value);
std::string read_from_file(std::string sensor_id, int num_datas);


#pragma pack(push, 1)
struct LogRecord {
    char sensor_id[32]; // supondo um ID de sensor de até 32 caracteres
    std::time_t timestamp; // timestamp UNIX
    double value; // valor da leitura
};
#pragma pack(pop)


// int write_in_file(std::string SensorName, std::string msg_time, std::string msg_value)
int write_in_file(std::string msg_sensor_id, std::string msg_time, std::string msg_value)
{
	// Abre o arquivo para leitura e escrita em modo binário e coloca o apontador do arquivo
	// apontando para o fim de arquivo
    std::fstream file(msg_sensor_id + ".dat", std::fstream::out | std::fstream::in | std::fstream::binary 
																	 | std::fstream::app); 
	// Caso não ocorram erros na abertura do arquivo
	if (file.is_open())
	{
		// Imprime a posição atual do apontador do arquivo (representa o tamanho do arquivo)
    file.seekg(0,std::ios_base::end);
		int file_size = file.tellg();

		// Recupera o número de registros presentes no arquivo
		int n = file_size/sizeof(LogRecord);
		std::cout << "Num records: " << n << " (file size: " << file_size << " bytes)" << std::endl;

		// Escreve 10 registros no arquivo
		// std::cout << "Writing 10 more records..." << std::endl;

		LogRecord rec;

    // rec.id = id++;
    strcpy(rec.sensor_id, msg_sensor_id.c_str());
    rec.timestamp = string_to_time_t(msg_time);
    rec.value = stod(msg_value);

    file.write((char*)&rec, sizeof(LogRecord));

		// Fecha o arquivo
		file.close();
	}
	else
	{
		std::cout << "Error opening file!" << std::endl;
		
	}
	return(0);
}

int read_file(std::string sensor_id, int num_records_req)
{
	// Abre o arquivo para leitura e escrita em modo binário e coloca o apontador do arquivo
	// apontando para o fim de arquivo
    std::fstream file(sensor_id + ".dat", std::fstream::out | std::fstream::in | std::fstream::binary 
																	 | std::fstream::app); 
	// Caso não ocorram erros na abertura do arquivo
	if (file.is_open())
	{
    
		// Imprime a posição atual do apontador do arquivo (representa o tamanho do arquivo)
    file.seekg(0,std::ios_base::end);
		int file_size = file.tellg();
    
		// Recupera o número de registros presentes no arquivo
		int n = file_size/sizeof(LogRecord);
		std::cout << "Num records: " << n << " (file size: " << file_size << " bytes)" << std::endl;
		
    if (num_records_req > n) num_records_req = n;

    file.seekp((n-num_records_req)*sizeof(LogRecord), std::ios_base::beg);

		// Le o registro selecionado
		LogRecord rec;

    std::string text_out = "" + num_records_req;

    for (int i=0;i<num_records_req; i++){
      
      file.read((char*)&rec, sizeof(LogRecord));      
      
      if (i != 0) text_out += ";";
      text_out += time_t_to_string(rec.timestamp);
      text_out += "|";
      text_out += std::to_string(rec.value);
    }

    text_out += "\r\n";

    // Imprime o registro
    std::cout << text_out << std::endl;

		// Fecha o arquivo
		file.close();
	}
	else
	{
		std::cout << "Error opening file!" << std::endl;
		
	}
	return(0);

}



using boost::asio::ip::tcp;
bool id_novo(const std::string& id){ return false;
}
bool log(const std::string& message){ return false;
}
bool get(const std::string& message){ return false;
    
}
class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    read_message();
  }

private:
  void read_message()
  {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, "\r\n",
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            std::istream is(&buffer_);
            std::string message(std::istreambuf_iterator<char>(is), {});
           // std::cout << "Received: " << message << std::endl;
           // write_message(message);
           std::string msg_type=message.substr(0,3);
           std::string id=message.substr(0,3);
              
            
           if(msg_type == "LOG"){
            if(!id_novo(id)){
            //Escrita no arquivo
            }
            if(id_novo(id)){
            //Cria arquivo para o novo sensor
            //Escrita no arquivo
            }
           }
           else if(msg_type == "GET"){
            if(!id_novo(id)){//Leitura do arquivo
            //Envio da informação para o cliente
            }
            
            if(id_novo(id)){
            //Envio de mensagem de erro para o cliente
            }
           }
          
          }
      
        });
  }

  void write_message(const std::string& message)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(message),
        [this, self, message](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            read_message();
          }
        });
  }

  tcp::socket socket_;
  boost::asio::streambuf buffer_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    accept();
  }

private:
  void accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: chat_server <port>\n";
    return 1;
  }

  /*

  boost::asio::io_context io_context;
  server s(io_context, std::atoi(argv[1]));
  io_context.run();
  // */

  //write_in_file("SENSOR_001", "2023-05-11T15:30:00", "78.5");

  read_file("SENSOR_001", std::atoi(argv[1]));

  return 0;
}