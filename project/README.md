# Description of Work (10 points)

This is new for Project 1. We ask you to include a file called `README.md` that contains a quick description of:

1. the design choices you made,
2. any problems you encountered while creating your solution, and
3. your solutions to those problems.

This is not meant to be comprehensive; less than 300 words will suffice.

We decided to separate the entire project into different phases. First, focus on getting the handshake working. After that we focused on getting the data transfer assuming no package lost to work.

Since we split the project into the handshake and reliable data transfer sections into separate parts to work on, high refactoring was needed due to how closely correlated both sections were. In order to improve readability,
we had to break handshake off into multiple sections and ensure initial sequence numbers and ACKs were properly passed to the main listen_loop.

In an attempt to ensure reliability and in-order data delivery, we implemented a receiver and sender buffer, along with four pointers to keep track of window size available. The sender buffer allowed us to track unacknowleged packets that may need to be retransmitted and how many packets were still in transmission. The receiver buffer allowed us to ensure in-order data delivery by doing linear scans for every sequence number between the last acknowledged packet's ACK and newest arrived packet's ACK.

We have encountered some problems along the way, one of them is that when we used `recvfrom` to receive the data, it would halt the program since both client and server were waiting for the other to send data. We solved this problem by using `select` to check if the socket is ready to receive data, and if not, we would send data instead.

Unfortunately, our program is far from being perfect. We will continue to work on it and make it better.
