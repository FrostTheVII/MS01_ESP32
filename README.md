# MS01_ESP32
Humidity reading using Sonoff MS01 with ESP32 (probably works with ESP8266)


Isto está meio instável, deve ser reescrito de forma a usar direitinho o FreeRTOS mas dada a natureza do protocolo de comunicação do MS01 à mínima diferença de timmings a leitura dá em erro, de qualquer forma assim está "usável".

Por vezes as primeiras tentativas após boot dão erro mas eventualmente (após +/- 10 ciclos de leitura) começam a sair medições corretas.

Outro pormenor está na accuracy das medições, a Sonoff não tem documentação nenhuma do seu protocolo e apenas com a ajuda do projeto Tasmota (https://tasmota.github.io/docs/) é que foi possível ler do sensor e ter uma "escala" mais ou menos usável. De qualquer forma os valores ao usar o Tasmota eram de 0.01% em ambiente seco e nunca passavam dos 50% com o sensor imerso parcialmente em água, por isso no MS01.Read() tem um remap entre 0 e o valor medido com ele imerso em água (variável wet_raw_value).

Por último, por vezes há umas medidas que saem muito ao lado das suas vizinhas sem que se tenha mexido no sensor, por isso o valor final de humidade relativa é uma média entre as últimas 3 medições, para desligar basta alterar o "smoothing" do ConvertHumidity de true para false.
