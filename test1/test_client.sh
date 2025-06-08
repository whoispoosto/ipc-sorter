IP_ADDR="0.0.0.0"
PORTNUM=31133

num_fragments=3

for ((i = 0 ; i < num_fragments ; i++)); do
  ./bin_client $IP_ADDR $PORTNUM &
done