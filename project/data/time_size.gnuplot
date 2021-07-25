set term wxt title 'Data Size vs Execution Time'
set ylabel "Time for transmitting the data to client (in seconds)"
set xlabel "Data Size (in KB)"
plot "./data/size_time_http1.dat" using 1:2 with lines title "HTTP-1", "./data/size_time_http1_1.dat" using 1:2 with lines title "HTTP-1.1"