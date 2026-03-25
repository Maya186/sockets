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

![Запуск с IP](C:\Projects\traceroute\1.png)


### 2. Запуск с доменным именем (google.com)

![Запуск с доменом](c:\Projects\traceroute\2.png)



### 3. Запуск стандартной tracert

[Запуск с IP](C:\Projects\traceroute\3.png)

![Запуск с доменом](c:\Projects\traceroute\4.png)

### 4. Wireshark: общий вид трафика

![Wireshark общий](c:\Projects\traceroute\5.png)



### 5. Wireshark: ICMP Echo Request (Type 8)

![Echo Request](c:\Projects\traceroute\6.png)


### 6. Wireshark: ICMP Time Exceeded (Type 11)

![Time Exceeded](c:\Projects\traceroute\7.png)



### 7. Wireshark: ICMP Echo Reply (Type 0)

![Echo Reply](c:\Projects\traceroute\8.png)

