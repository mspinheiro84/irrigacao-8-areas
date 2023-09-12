# _Projeto de irrigação de 8 áreas independentes_

Este projeto visa criar um passo a passo, via commits de README, para automação de 8 areas para irrigação, com auxílio de um reservatório com bomba d'água para quando o sensor detectar a faltar água.

Assim como a ativação e comunicação via aplicativo.

## Task contador

A task led foi modificada para blink e foi implementado uma task contadora para contar no número de acionamentos do botão e suspender a task blink em 10 acionamentos e libera-la no 13ª

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
