#ifndef TYPES_H
#define TYPES_H

enum Type
{
    RollCall,
    Status,
    Move,
};

struct Position
{
    float x;
    float y;
    float z;
};

struct Velocity
{
    float vx;
    float vy;
    float vz;
};

struct RollCall
{
    int leader_id;
};

struct Status
{
    int leader_id;
};

struct Move{
    int leader_id;
    Position position;
    Velocity velocity;
};

struct Message
{
    Type type;
    struct RollCall rollcall;
    struct Status status;
    struct Move move;
};

struct RollCallResponse
{
    int member_id;
    Position position;
};

struct StatusResponse
{
    int member_id;
    Position position;
    Velocity velocity;
    float fuel;
};

struct MoveResponse{
    int member_id;
};

struct Response
{
    Type type;
    RollCallResponse rollcall_response;
    StatusResponse status_response;
    MoveResponse move_response;
};

struct Member
{
    int id;
    Position position;
    Velocity velocity;
    float fuel;
};

#endif // TYPES_H
