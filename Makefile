# Makefile for server and client programs in C++

CXX = g++
CXXFLAGS = -Wall

# Nome dos executáveis
SERVER_EXEC = server
CLIENT_EXEC = client

# Alvos para compilação
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Compilação do servidor
$(SERVER_EXEC): server.cpp
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXEC) server.cpp

# Compilação do cliente
$(CLIENT_EXEC): client.cpp
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXEC) client.cpp

# Alvo para limpar arquivos binários
clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)
