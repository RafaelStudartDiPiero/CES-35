#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include "types.h"

#define SERVER_PORT 8080
#define WAIT_SECONDS 6 

int main(int argc, char **argv)
{
    printf("Starting Member %d\n", atoi(argv[2]));

    // Starting Member
    Member member;
    member.id = atoi(argv[2]);
    member.position.x = atoi(argv[3]);
    member.position.y = atoi(argv[4]);
    member.position.z = atoi(argv[5]);
    member.velocity.vx = 0;
    member.velocity.vy = 0;
    member.velocity.vz = 0;
    member.fuel = 100;

    int s;
    struct hostent *h;
    // Defining IP Address
    struct sockaddr_in channel;

    h = gethostbyname(argv[1]);
    if (!h)
    {
        printf("gethostbyname failed to locate %s\n", argv[1]);
        exit(-1);
    }

    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
    {
        printf("socket call failed");
        exit(-1);
    }

    memset(&channel, 0, sizeof(channel));
    channel.sin_family = AF_INET;
    memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
    channel.sin_port = htons(SERVER_PORT);

    int c = connect(s, (struct sockaddr *)&channel, sizeof(channel));
    if (c < 0)
    {
        printf("connect failed");
        exit(-1);
    }

    // Listen for messages continuously
    while (1)
    {
        Message msg;
        int bytes = read(s, &msg, sizeof(Message));
        if (bytes <= 0)
        {
            // Handle disconnection or error
            printf("Server closed the connection or an error occurred. Exiting.\n");
            close(s);
            exit(0);
        }

        switch (msg.type)
        {
        case RollCall:
        {
            printf("Received rollcall from %d.\n", msg.rollcall.leader_id);

            // OPTIONAL SLEEP TO TEST TIMEOUT
            // std::this_thread::sleep_for(std::chrono::seconds(WAIT_SECONDS));

            Response response;
            response.type = RollCall;
            response.rollcall_response.member_id = member.id;
            response.rollcall_response.position = member.position;
            write(s, &response, sizeof(response));
            break;
        }
        case Status:
        {
            printf("Received status from %d.\n", msg.status.leader_id);

            // OPTIONAL SLEEP TO TEST TIMEOUT
            // std::this_thread::sleep_for(std::chrono::seconds(WAIT_SECONDS));

            Response response;
            response.type = Status;
            response.status_response.member_id = member.id;
            response.status_response.position = member.position;
            response.status_response.velocity = member.velocity;
            response.status_response.fuel = member.fuel;
            write(s, &response, sizeof(response));
            break;
        }
        case Move:
        {
            printf("Received move from %d.\n", msg.move.leader_id);

            // OPTIONAL SLEEP TO TEST TIMEOUT
            std::this_thread::sleep_for(std::chrono::seconds(WAIT_SECONDS));

            Response response;
            response.type = Move;
            response.move_response.member_id = member.id;
            write(s, &response, sizeof(response));
            member.position = msg.move.position;
            member.velocity = msg.move.velocity;
            member.fuel = member.fuel - 10;
            break;
        }
        default:
            printf("Received unknown request");
            break;
        }
    }
}