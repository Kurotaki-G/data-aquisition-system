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

using boost::asio::ip::tcp;

//funções auxiliares
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

// struct dos dados, conforme o enunciado
#pragma pack(push, 1)
struct LogRecord {
    char sensor_id[32]; // supondo um ID de sensor de até 32 caracteres
    std::time_t timestamp; // timestamp UNIX
    double value; // valor da leitura
};
#pragma pack(pop)

//declaração das funções próprias
int write_in_file(std::string msg_sensor_id, std::string msg_time, std::string msg_value);
bool id_novo(std::string sensor_id);
std::string read_from_file(std::string sensor_id, int num_records_req);


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
            std::vector<std::string> new_message=split_string(message,"|");
           std::string msg_type=new_message[0];
           std::string id=new_message[1];
           if(msg_type=="LOG"){
              std::string msg_time = new_message[2];
              std::string msg_value = new_message[3];
              write_in_file(id, msg_time, msg_value); //escreve no arquivo dedicado ao sensor. Caso o arquivo não exista, será criado automaticamente
              write_message("OK"); // resposta de confirmação, para manter a conexão ativa.
          }
          else if(msg_type=="GET"){
            if(!id_novo(id)){
              int n_infos =stoi(new_message[2]);
              std::string rec = read_from_file(id, n_infos);
              write_message(rec);
            }
            else{
              //Envio de mensagem de erro para o cliente
              std::string error = "ERROR|INVALID_SENSOR_" + id + "\r\n";
              write_message(error);
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

  boost::asio::io_context io_context;
  server s(io_context, std::atoi(argv[1]));
  io_context.run();

  return 0;
}



/// FUNÇÕES PRÓPRIAS

bool id_novo(std::string sensor_id){
  std::ofstream ofs;
	ofs.open(sensor_id+".dat",std::ios_base::in); //tenta abrir
	if(!ofs)
	{//caso não exista, avisa que é novo
    ofs.close();
    return true;
	}
  return false;
}

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

		LogRecord rec;

    // rec.id = id++;
    strcpy(rec.sensor_id, msg_sensor_id.c_str());
    rec.timestamp = string_to_time_t(msg_time);
    rec.value = stod(msg_value);

    file.write((char*)&rec, sizeof(LogRecord));

		// Imprime o registro
		std::cout << "Id: "  << rec.sensor_id << " - Timestamp: " << time_t_to_string(rec.timestamp) << " - Value: "<< rec.value << std::endl;

		// Fecha o arquivo
		file.close();
	}
	else
	{
		std::cout << "Error opening file!" << std::endl;
		
	}
	return(0);
}

std::string read_from_file(std::string sensor_id, int num_records_req)
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

    std::string text_out = std::to_string(num_records_req);

    for (int i=0;i<num_records_req; i++){
      
      file.read((char*)&rec, sizeof(LogRecord));      
      
      text_out += ";";
      text_out += time_t_to_string(rec.timestamp);
      text_out += "|";
      text_out += std::to_string(rec.value);
    }

    text_out += "\r\n";

    // Imprime o registro
    // std::cout << "Text readed: " << text_out << std::endl;

		// Fecha o arquivo
		file.close();

    return text_out;

	}
	else
	{
		std::cout << "Error opening file!" << std::endl;
    return "ERROR";
		
	}
	return(0);

}