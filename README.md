# IRC - Redes

Projeto da Disciplina **SSC0142 - Redes de Computadores (2020)**, no qual implementaremos uma versão simplificada do protocolo IRC (Internet Relay Chat). Sua definição pode ser encontrada em: <https://tools.ietf.org/html/rfc1459.> com uma interface gráfica

## Autores

- Vitor Santana Cordeiro, 10734345
- Marcelo Isaias de Moraes Junior, 10550218
- Joao Vitor Silva Ramos, 10734769

> O código foi compilado utilizando o compilador `gcc versão 7.5.0`, com `target = Linux GNU x86_64`. O código é feito em `C++11`, usando a `flag -std=c++11` na compilação.

## Como usar

Para testar o código, compile-o com o comando `make all`.<br/><br/>
Depois, digite `./server` em um terminal. O servidor será iniciado na porta 9001 (essa configuração pode ser mudada por meio do `#define PORT` no arquivo `server.cpp`).<br/><br/>
Primeiramente, caso queira utilizar da interface gráfica no cliente, é necessário apenas digitar o comando `python3 client.py`.
Caso contrário, para iniciar apenas o programa cliente no terminal, simplesmente execute o comando `./client`. Siga as intruções dadas pelo terminal e conecte a um dos servidores disponíveis no arquivo `dns.txt`. Você pode adicionar novos servidores por meio do mesmo.<br/><br/>
Para permitir conexões fora da rede local, você deve ou fazer port-forwarding em seu ponto de acesso ou rodar o seguinte comando em outro terminal:
> ssh -R 9001:localhost:9001 serveo.net.
<!-- -->
Ele comecará um tunelamento SSH reverso que te permitirá acessar o servidor pelo DNS `main.ggc`.<br/>
Caso decidir fazer port-forwarding, lembre-se de registrar seu IP e porta no arquivo `dns.txt`.

## Comandos disponíveis

- `/list`: Lista todos os servidores conhecidos pelo nosso "DNS" (arquivo `dns.txt`);
- `/connect`: Estabelece a conexão com o servidor especificado;
- `/ping`: O servidor retorna "pong" assim que receber a mensagem;
- `/nickname`: Muda o nickname atual do cliente para o novo especificado;
- `/join`: Entra no canal especificado;
- `/mute`: (Comando de administrador) Impede o usuário especificado de mandar mensagens no canal;
- `/unmute`: (Comando de administrador) Retira o estado criado pelo /mute;
- `/kick`: (Comando de administrador) Expulsa o usuário especificado do canal atual e o manda para o canal #general;
- `/invite`: (Comando de administrador) manda um convite para o usuário especificado entrar em um canal (caso o canal seja privado, só usuários que receberam esse convite podem entrar);
- `/whois`: (Comando de administrador) Recebe o IP do usuário especificado (esse comando só funciona se o servidor **NÃO** estiver utilizando tunelamento SSH, caso contrário sempre mostrará o IP `127.0.0.1` para qualquer usuário);
- `/quit`: O cliente fecha a conexão e fecha a aplicação.

> Para mandar mensagens no servidor, apenas digite normalmente no terminal (ou caso esteja utilizando a interface gráfica, digite na área de texto inferior) e aperte **ENTER**.

## Interface gráfica

Para utilizar a interface gráfica, o programa python3 (versão mínima `3.7`) e a sua biblioteca `tkinter` devem estar ser instalados (e atualizados). Caso alguma outra biblioteca falte no seu dispositivo (o que é muito improvável, já que são bibliotecas padrões do Python), por favor as instale.
