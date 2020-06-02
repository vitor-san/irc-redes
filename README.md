# IRC - Redes

Projeto da Disciplina **SSC0142 - Redes de Computadores (2020)**, no qual implementaremos uma versão simplificada do protocolo IRC (Internet Relay Chat). Sua definição pode ser encontrada em: <https://tools.ietf.org/html/rfc1459.>

## Autores

-   Vitor Santana Cordeiro, 10734345
-   Marcelo Isaias de Moraes Junior, 10550218
-   Joao Vitor Silva Ramos, 10734769

> O código foi compilado utilizando o compilador `gcc versão 7.5.0`, com `target = Linux GNU x86_64`. O código é feito em `C++11`, usando a `flag -std=c++11` na compilação.

## Como usar

Para testar o código, compile-o com o comando `make all`.<br/><br/>
Depois, digite `./server` em um terminal. O servidor será iniciado na porta 9001 (essa configuração pode ser mudada por meio do `#define PORT` no arquivo `server.cpp`).<br/><br/>
Para iniciar o programa cliente, simplesmente execute o comando `./client`. Siga as intruções dadas pelo terminal e conecte a um dos servidores disponíveis no arquivo `dns.txt`. Você pode adicionar novos servidores por meio do mesmo.<br/><br/>
Para permitir conexões fora da rede local, você deve ou fazer port-forwarding em seu ponto de acesso ou rodar o seguinte comando em outro terminal:
> ssh -R 9001:localhost:9001 serveo.net
Ele comecará um tunelamento SSH reverso que te permitirá acessar o servidor pelo DNS `main.ggc`.<br/>
Caso decidir fazer port-forwarding, lembre-se de registrar seu IP e porta no arquivo `dns.txt`.

## Comandos disponíveis

-   `/list`: Lista todos os servidores conhecidos pelo nosso "DNS" (arquivo `dns.txt`);
-   `/connect`: Estabelece a conexão com o servidor especificado;
-   `/ping`: O servidor retorna "pong" assim que receber a mensagem;
-   `/nickname`: Muda o nickname atual do cliente para o novo especificado;
-   `/quit`: O cliente fecha a conexão e fecha a aplicação.

> Para mandar mensagens no servidor, apenas digite normalmente no terminal e aperte **ENTER**.

## Aviso

Utilizar redirecionamento do _stdin_ para um arquivo de texto pode causar comportamentos indesejáveis do programa. **NÃO** faça isso para testá-lo!
