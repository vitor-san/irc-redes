# IRC - Redes

Projeto da Disciplina **SSC0142 - Redes de Computadores (2020)**, no qual implementaremos uma versão simplificada do protocolo IRC (Internet Relay Chat). Sua definição pode ser encontrada em: https://tools.ietf.org/html/rfc1459.

## Autores:
- Vitor Santana Cordeiro, 10734345
- Marcelo Isaias de Moraes Junior, 10550218
- Joao Vitor Silva Ramos, 10734769

> O código foi compilado utilizando o compilador `gcc versão 7.5.0`, com `target = Linux GNU x86_64`. O código é feito em `C++11`, usando a `flag -std=c++11` na compilação.

## Como usar?
Para testar o código, compile o código com o comando `make all`.
Depois, digite `./server [port]` em um terminal, sendo port uma porta disponível (e.g. 9001), para abrir um servidor.
Por fim, para enviar as mensagens, utilize, em outro terminal, o comando `./client [localhost] [port]`, sendo port a mesma usada anteriormente. Por fim, basta enviar as mensagens, e o servidor irá enviá-las de volta (ou seja, este é um **servidor de echo**).