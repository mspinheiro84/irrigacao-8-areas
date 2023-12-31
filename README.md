# _Projeto de irrigação de 8 áreas independentes_

Este projeto visa criar um passo a passo, via commits de README, para automação de 8 areas para irrigação, com auxílio de um reservatório com bomba d'água para quando o sensor detectar a faltar água.

Assim como a ativação e comunicação via aplicativo.

## JSON resumo quando atualizar estados

Foi adicionado uma publicação no MQTT com topico resumo sempre que houver uma alteração no estado dos atuadores.


## Atualização dos estados dos atuadores remotamente

O JSON recebido via mqtt é lido e validado e os estados dos atuadores são atualizados. No mesmo formato o ESP32 envia um resumo dos status dos atuadores e sensor

## Implemetação do JSON para mqtt

Foi implementado um JSON para enviar e receber informação via mqtt.

## Tratamento de dados do mqtt

Foi implementada a função mqtt_app_data para tratar os dados recebidos pelo protocolo mqtt.

## Implementação da Struct Dados

Foi criada a struct Dados, informações que serão passada para o servidor mqtt. Também foi implementada uma task para atualizar a contagem do sensor, em um minuto, na struct Dados.

## SNTP - Sincronização online de relógio

Foi implementado de uma task para sincronização online da data e hora.

## Implementação da função mqtt_app_reconnect

Foi implementado a função mqtt_app_reconnect na biblioteca mqtt.c. Ação necessária para reconexão com o broker mqtt.

## Implementação da função reconectaWifi

Foi implementado a função reconectaWifi na biblioteca wifi.c. Além de realizar a reconexão com wifi ela funcionará como validação da conexão para a requisição http e o mqtt.

## MQTT

Foi implementado o protocolo MQTT tcp para comunicação remota.

## Extração da data e hora

Foi criado um strut DataHora para receber os dados da data, hora e fuso. Também foi implementada uma função para fazer a extração da data, hora e fuso do JSON

## Alteração na obtenção do JSON da request

Foi alterada as funcionalidades do request para ficar independente do API usada e retornar o JSON para ser tratado na função que chamou a requisição.

## Separação das funcionalidades WIFI

As funcionalidades da conexão com wifi foram separados em arquivo próprio, wifi.c.
O arquivo foi baseado no exemplo "station" da expressif do github.
https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station

## Separação das funcionalidades HTTP Request

As funcionalidades de requisição http client foram separadas em arquivo próprio, http_client.c.

## HTTP Request

Implementação de um request pela task HTTP client, com consumo de API de data e hora.

## Conexão WIFI

Implementação da conexão com wifi. Foi adcionado uma componente de conexão wifi no CMakeList.txt
"set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)"
Para conexão é necessário adicionar o SSID e senha da rede no menuconfig.

## Aumento da usStackDepth

Aumento da usStackDepth nas task por estouro da pilha durante a execução.

## Task contador

A task led foi modificada para blink e foi implementado uma task contadora para contar no número de acionamentos do botão e suspender a task blink em 10 acionamentos e libera-la no 13ª.

## Semaphore

Foi implementado um semáforo para sincronizar a interrupção do botão e a execução de uma tarefa.

## Interrupção

Foi adicionado uma interrpção no botão para evitar perder pulso no momento de delay da função que trata o botão.

## Button

Foi implentado um botão no pino 21, representa um sensor de pulso, que fará o led piscar uma vez. Também foi implementada uma função com as configurações dos pinos.

## ESP_LOG

Foi implementada a funcionalidade de log.

## Task

Foi introduzido o conceito de task do FreeRTOS, criando tarefas que serão executadas de forma paralela.

## Blink

Neste passo foi construido a função blink, o "hello world" dos microcontroladres, onde o pino 2 da placa, que já vem com um LED integrado, foi configurado como saída.
