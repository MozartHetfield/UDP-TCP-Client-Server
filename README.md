# UDP-TCP-Client-Server
A client-server TCP/UDP application with topics and subscribtions

Usage:

./server <port>
  
./subscriber <id> <server_ip> <port>
  
python3 udp_client.py --source-port <port> --input_file three_topics_payloads.json --mode random --delay 2000 <server_ip> <server_port> 
