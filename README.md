# soq_sec
Victor Machado Gonzaga - nusp 10692033
Troque informações sigilosas de forma segura(todo)

## Como usar:
Clone o repositório e na pasta gerada, use o make para compilar o código.

Iniciar servidor:
```./soq_sec server <ip> <port>```

Iniciar cliente:
```./soq_sec client <ip> <port>```

## Comandos disponíveis:

• /connect - Estabelece a conexão com o servidor;

• /quit - O cliente fecha a conexão e fecha a aplicação;

• /ping - O servidor retorna "pong"assim que receber a mensagem

• /join nomeCanal - Entra no canal;

• /nickname apelidoDesejado - O cliente passa a ser reconhecido pelo apelido especificado;

• /ping - O servidor retorna "pong"assim que receber a mensagem.

Comandos apenas para administradores de canais:

• /kick nomeUsurio - Fecha a conexão de um usuário especificado

• /mute nomeUsurio - Faz com que um usuário não possa enviar mensagens neste canal

• /unmute nomeUsurio - Retira o mute de um usuário.

• /whois nomeUsurio - Retorna o endereço IP do usuário apenas para o administrador

## End-to-end crypto IRC proof of concept, free of external libraries (to do)
ECDHE - Elliptic curve diffie hellman ephemeral public keys, with curve25519 parameters

Implemented as in: https://tools.ietf.org/html/rfc7539

chacha20 - Stream cipher for message encryption

poly1305 - MAC
