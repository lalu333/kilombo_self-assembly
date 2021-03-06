#include<kilombo.h>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944
#endif

#define MAXN 30
#define RB_SIZE 32   // Ring buffer size. Choose a power of two for faster code

enum BOTTYPE {LAST, FOLLOWER, LEADER};
enum BOTSTATES {START,WAIT_TO_MOVE,MOVE_OUTSIDE,MOVE_INSIDE,JOINED_SHAPE};

// declare variables

typedef struct {
  uint16_t ID;
  uint8_t dist;
  uint8_t gradient;
  uint32_t timestamp;
  uint8_t n_bot_state;

  int16_t pos_x;
  int16_t pos_y;
  uint8_t localized;

} Neighbor_t;

typedef struct {
    message_t msg;
    distance_measurement_t dist;
} received_message_t;

typedef struct 
{
    Neighbor_t neighbors[MAXN];
    int N_Neighbors;    
    uint8_t gradient_value;

    message_t msg;  
    received_message_t RXBuffer[RB_SIZE];
    uint8_t RXHead, RXTail; 

    uint8_t bot_type;
    uint8_t bot_state;
    uint8_t move_type;
    char message_lock;

    uint8_t prev;

    int16_t pos_x;
    int16_t pos_y;
    uint8_t localized;
    int8_t accurate;
    int timer;

    unsigned char bmpPixel[256][256];
    int bmp_x;
    int bmp_y;

} USERDATA;

extern USERDATA *mydata;

// Ring buffer operations. Taken from kilolib's ringbuffer.h
// but adapted for use with mydata->

// Ring buffer operations indexed with head, tail
// These waste one entry in the buffer, but are interrupt safe:
//   * head is changed only in popfront
//   * tail is changed only in pushback
//   * RB_popfront() is to be called AFTER the data in RB_front() has been used
//   * head and tail indices are uint8_t, which can be updated atomically
//     - still, the updates need to be atomic, especially in RB_popfront()

#define RB_init() {	\
    mydata->RXHead = 0; \
    mydata->RXTail = 0;\
}

#define RB_empty() (mydata->RXHead == mydata->RXTail)

#define RB_full()  ((mydata->RXHead+1)%RB_SIZE == mydata->RXTail)

#define RB_front() mydata->RXBuffer[mydata->RXHead]

#define RB_back() mydata->RXBuffer[mydata->RXTail]

#define RB_popfront() mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;


#define RB_pushback() {\
    mydata->RXTail = (mydata->RXTail+1)%RB_SIZE;\
    if (RB_empty())\
      { mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;	\
	/*printf("Full.\n"); */}			\
  }

/*
#define RB_pushback() {\
    mydata->RXTail = (mydata->RXTail+1)%RB_SIZE;\
    if (RB_empty())\
      { mydata->RXHead = (mydata->RXHead+1)%RB_SIZE;	\
  }
*/
