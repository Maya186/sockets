# 🌐 Traceroute на Raw Sockets (C++)

**Простой аналог утилиты traceroute/tracert**

---

## Описание

Программа для трассировки маршрута с использованием ICMP и raw sockets.


traceroute.exe 8.8.8.8

traceroute.exe google.com

traceroute.exe -d 8.8.8.8  # с обратным DNS

## Результаты работы

### 1. Запуск с IP-адресом (8.8.8.8)

![Запуск с IP](1.png)


### 2. Запуск с доменным именем (google.com)

![Запуск с доменом](2.png)



### 3. Запуск стандартной tracert

[Запуск с IP](3.png)

![Запуск с доменом](4.png)

### 4. Wireshark: общий вид трафика

![Wireshark общий](5.png)



### 5. Wireshark: ICMP Echo Request (Type 8)

![Echo Request](6.png)


### 6. Wireshark: ICMP Time Exceeded (Type 11)

![Time Exceeded](7.png)



### 7. Wireshark: ICMP Echo Reply (Type 0)

![Echo Reply](8.png)

