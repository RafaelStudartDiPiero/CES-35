#include <sys/fcntl.h>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include "types.h"

#define SERVER_PORT 8080
#define QUEUE_SIZE 10
#define BUF_SIZE 256
#define TIMEOUT_SECONDS 5
#define TIMEOUT_CRITICAL_SECONDS 10

void handleResponse(Response &response, Member &leader, std::vector<Member> &squad)
{
    if (response.type == RollCall)
    {
        printf("Received rollcall response from %d.\n", response.rollcall_response.member_id);
        if (response.rollcall_response.member_id != leader.id)
        {
            Member squad_member;
            squad_member.id = response.rollcall_response.member_id;
            squad_member.position = response.rollcall_response.position;

            bool member_found = false;
            for (Member &member : squad)
            {
                if (member.id == response.rollcall_response.member_id)
                {
                    member.position = squad_member.position;
                    member_found = true;
                    printf("Updated squad member status: id=%d, x=%f, y=%f, z=%f\n", squad_member.id, squad_member.position.x, squad_member.position.y, squad_member.position.z);
                    break;
                }
            }
            if (!member_found)
            {
                // Member not found in the squad, so add them
                squad.push_back(squad_member);
                printf("Added new squad member: id=%d, x=%f, y=%f, z=%f\n", squad_member.id, squad_member.position.x, squad_member.position.y, squad_member.position.z);
            }
        }
    }
    else if (response.type == Status)
    {
        printf("Received status response from %d.\n", response.status_response.member_id);
        bool member_found = false;
        for (Member &member : squad)
        {
            if (member.id == response.status_response.member_id)
            {
                member.position = response.status_response.position;
                member.velocity = response.status_response.velocity;
                member.fuel = response.status_response.fuel;
                member_found = true;
                printf("Updated squad member status: id=%d, x=%f, y=%f, z=%f, vx=%f, vy=%f, vz=%f, fuel=%f\n", response.status_response.member_id, response.status_response.position.x, response.status_response.position.y, response.status_response.position.z, response.status_response.velocity.vx, response.status_response.velocity.vy, response.status_response.velocity.vz, response.status_response.fuel);
                break;
            }
        }
        if (!member_found)
        {
            // Member not found in the squad, so add them
            Member squad_member;
            squad_member.id = response.status_response.member_id;
            squad_member.position = response.status_response.position;
            squad_member.velocity = response.status_response.velocity;
            squad_member.fuel = response.status_response.fuel;
            squad.push_back(squad_member);
            printf("Added new squad member: id=%d, x=%f, y=%f, z=%f\n", squad_member.id, squad_member.position.x, squad_member.position.y, squad_member.position.z);
        }
    }
    else if (response.type == Move)
    {
        printf("Received move response from %d.\n", response.move_response.member_id);
        printf("Moved squad member id=%d\n", response.move_response.member_id);
    }
}

int main(int argc, char *argv[])
{
    printf("Starting Leader\n");

    // Starting Leader
    Member leader;
    leader.id = 0;
    leader.position.x = 0;
    leader.position.y = 0;
    leader.position.z = 0;
    leader.velocity.vx = 0;
    leader.velocity.vy = 0;
    leader.velocity.vz = 0;
    leader.fuel = 100;
    // Array of Active Members of Squad
    std::vector<Member> squad;

    int s, b, l, sa, on = 1;
    // IP Adress
    struct sockaddr_in channel;

    // Single Member Squad
    int squad_socket;

    // Binds Address to Socket
    memset(&channel, 0, sizeof(channel));
    channel.sin_family = AF_INET;
    channel.sin_addr.s_addr = htonl(INADDR_ANY);
    channel.sin_port = htons(SERVER_PORT);

    // Creates Socket
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
    {
        printf("socket call failed");
        exit(-1);
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

    b = bind(s, (struct sockaddr *)&channel, sizeof(channel));
    if (b < 0)
    {
        printf("bind call failed");
        exit(-1);
    }

    l = listen(s, QUEUE_SIZE);
    if (l < 0)
    {
        printf("listen call failed");
        exit(-1);
    }

    sa = accept(s, 0, 0);
    squad_socket = sa;
    bool loop = true;

    while (loop)
    {
        char input[BUF_SIZE];
        printf("Enter command (e.g., 'rollcall', 'status member-id', 'move member-id x y z vx vy vz', 'exit'): ");
        fgets(input, sizeof(input), stdin);
        Message message;
        bool message_sent;
        if (strcmp(input, "rollcall\n") == 0)
        {
            message.type = RollCall;
            message.rollcall.leader_id = leader.id;

            write(squad_socket, &message, sizeof(message));
            message_sent = true;

            if (message_sent)
            {
                // Configure Timer
                struct timeval timeout;
                timeout.tv_sec = TIMEOUT_SECONDS;
                timeout.tv_usec = 0;

                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(squad_socket, &read_fds);

                // Check timer
                int select_result = select(squad_socket + 1, &read_fds, NULL, NULL, &timeout);

                if (select_result == 0)
                {
                    printf("Rollcall timed out. No response received.\n");
                    printf("Waiting for more %d seconds\n", TIMEOUT_CRITICAL_SECONDS);

                    struct timeval extended_timeout;
                    extended_timeout.tv_sec = TIMEOUT_CRITICAL_SECONDS;
                    extended_timeout.tv_usec = 0;

                    fd_set read_fds;
                    FD_ZERO(&read_fds);
                    FD_SET(squad_socket, &read_fds);

                    // Aditional Time
                    select_result = select(squad_socket + 1, &read_fds, NULL, NULL, &extended_timeout);

                    if (select_result == 0)
                    {
                        printf("Full fail rollcall timed out. No response received.\n");
                    }
                    else if (select_result > 0)
                    {
                        Response response;
                        read(sa, &response, sizeof(Response));
                        handleResponse(std::ref(response), std::ref(leader), std::ref(squad));
                    }
                }
                else if (select_result > 0)
                {
                    // Waiting Response from Member
                    Response response;
                    read(sa, &response, sizeof(Response));
                    handleResponse(std::ref(response), std::ref(leader), std::ref(squad));
                }
                else
                {
                    perror("select");
                }
            }
        }
        else if (strncmp(input, "status", 6) == 0)
        {
            int member_id;
            if (sscanf(input, "status %d", &member_id) == 1)
            {
                message.type = Status;
                message.status.leader_id = leader.id;

                bool member_found = false;
                for (Member &member : squad)
                {
                    if (member.id == member_id)
                    {
                        member_found = true;
                        break;
                    }
                }

                if (member_found)
                {
                    write(squad_socket, &message, sizeof(message));
                    message_sent = true;
                }
                else
                {
                    printf("This member is not in squad. Make a Roll-Call first\n");
                }
            }
            else
            {
                printf("Invalid 'status' command format. Use 'status member-id'.\n");
            }

            if (message_sent)
            {
                // Configure Timer
                struct timeval timeout;
                timeout.tv_sec = TIMEOUT_SECONDS;
                timeout.tv_usec = 0;

                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(squad_socket, &read_fds);

                // Check timer
                int select_result = select(squad_socket + 1, &read_fds, NULL, NULL, &timeout);

                if (select_result == 0)
                {
                    printf("Rollcall timed out. No response received.\n");
                    // Remove member if the timeout occurs
                    for (auto it = squad.begin(); it != squad.end(); ++it)
                    {
                        if (it->id == member_id)
                        {
                            squad.erase(it);
                            printf("Removed member %d.\n", member_id);
                            break;
                        }
                    }
                }
                else if (select_result > 0)
                {
                    // Waiting Response from Member
                    Response response;
                    read(sa, &response, sizeof(Response));
                    handleResponse(std::ref(response), std::ref(leader), std::ref(squad));
                }
                else
                {
                    perror("select");
                }
            }
        }
        else if (strncmp(input, "move", 4) == 0)
        {
            // Parse 'move member-id x y z vx vy vz' command
            int member_id;
            float x, y, z, vx, vy, vz;
            if (sscanf(input, "move %d %f %f %f %f %f %f", &member_id, &x, &y, &z, &vx, &vy, &vz) == 7)
            {
                message.type = Move;
                message.move.leader_id = leader.id;
                message.move.position.x = x;
                message.move.position.y = y;
                message.move.position.z = z;
                message.move.velocity.vx = vx;
                message.move.velocity.vy = vy;
                message.move.velocity.vz = vz;

                bool member_found = false;
                for (Member &member : squad)
                {
                    if (member.id == member_id)
                    {
                        member_found = true;
                        break;
                    }
                }

                if (member_found)
                {
                    write(squad_socket, &message, sizeof(message));
                    message_sent = true;
                }
                else
                {
                    printf("This member is not in squad. Make a Roll-Call first\n");
                }
            }
            else
            {
                printf("Invalid 'move' command format. Use 'move member-id x y z vx vy vz'.\n");
            }

            if (message_sent)
            {
                // Configure Timer
                struct timeval timeout;
                timeout.tv_sec = TIMEOUT_SECONDS;
                timeout.tv_usec = 0;

                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(squad_socket, &read_fds);

                // Check timer
                int select_result = select(squad_socket + 1, &read_fds, NULL, NULL, &timeout);

                if (select_result == 0)
                {
                    printf("Rollcall timed out. No response received.\n");
                    // Remove member if the timeout occurs
                    for (auto it = squad.begin(); it != squad.end(); ++it)
                    {
                        if (it->id == member_id)
                        {
                            squad.erase(it);
                            printf("Removed member %d.\n", member_id);
                            break;
                        }
                    }
                }
                else if (select_result > 0)
                {
                    // Waiting Response from Member
                    Response response;
                    read(sa, &response, sizeof(Response));
                    handleResponse(std::ref(response), std::ref(leader), std::ref(squad));
                }
                else
                {
                    perror("select");
                }
            }
        }
        else if (strcmp(input, "exit\n") == 0)
        {
            printf("Closing server\n");
            loop = false;
        }
        else
        {
            printf("Invalid command. Use 'rollcall', 'status member-id', 'move member-id x y z vx vy vz' or 'exit'\n");
        }
        message_sent = false;
    }
}