IP_ADDR="0.0.0.0"
PORTNUM=31133

num_fragments=8 # More clients than needed

for ((i = 0 ; i < num_fragments ; i++)); do
  ./bin_client $IP_ADDR $PORTNUM &
done
