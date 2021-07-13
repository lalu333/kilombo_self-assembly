#include "blink.h" 
#include <math.h>
#include "myLocation.c"
//#include "mybmp.c"

enum {STOP,LEFT,RIGHT,FORWARD};

#define TOO_CLOSE_DISTANCE 40
#define DESIRED_DISTANCE 44

#define CUTOFF 60 //neighbors further away are ignored. (mm)

REGISTER_USERDATA(USERDATA)

// rainbow colors
uint8_t colors[] = {
  RGB(0,0,0),  //0 - off
  RGB(2,0,0),  //1 - red
  RGB(2,1,0),  //2 - orange
  RGB(2,2,0),  //3 - yellow
  RGB(1,2,0),  //4 - yellowish green
  RGB(0,2,0),  //5 - green
  RGB(0,1,1),  //6 - cyan
  RGB(0,0,1),  //7 - blue
  RGB(1,0,1),  //8 - purple
  RGB(3,3,3)   //9  - bright white
};

void set_bot_state(int state)
{
  mydata->bot_state = state;
}

int get_bot_state(void)
{
  return mydata->bot_state;
}

void set_move_type(int type)
{
  mydata->move_type = type;
}

int get_move_type(void)
{
  return mydata->move_type;
}

// message rx callback function. Pushes message to ring buffer.
void rxbuffer_push(message_t *msg, distance_measurement_t *dist) {
    received_message_t *rmsg = &RB_back();
    rmsg->msg = *msg;
    rmsg->dist = *dist;
    RB_pushback();
}

message_t *message_tx() {
    if (mydata->message_lock||(mydata->gradient_value==UINT8_MAX&&get_bot_state()==START))
        return '\0';
    else
        return &mydata->msg;
}

void setup_message(void)
{
  mydata->message_lock = 1;  //don't transmit while we are forming the message
  mydata->msg.type = NORMAL;
  mydata->msg.data[0] = kilo_uid & 0xff;         // 0 low  ID
  mydata->msg.data[1] = kilo_uid >> 8;           // 1 high ID
  mydata->msg.data[2] = mydata->localized;     // 2 number of neighbors


  mydata->msg.data[3] = mydata->gradient_value&0xFF;      // 3 low  byte of gradient value
  //mydata->msg.data[4] = (mydata->gradient_value>>8)&0xFF; // 4 high byte of gradient value
  mydata->msg.data[4] = get_bot_state();     // 5 bot state

  /*int tmp5=(int)mydata->pos_x;
  int tmp6=(int)((mydata->pos_x-(int)mydata->pos_x)*100);
  if(mydata->pos_x<0) tmp6*=-1;
  int tmp7=(int)mydata->pos_y;
  int tmp8=(int)((mydata->pos_y-(int)mydata->pos_y)*100);
  if(mydata->pos_y<0) tmp8*=-1;
  mydata->msg.data[5]=tmp5&0xff;
  mydata->msg.data[6]=tmp6&0xff;
  mydata->msg.data[7]=tmp7&0xff;
  mydata->msg.data[8]=tmp8&0xff;*/

  mydata->msg.data[5]=mydata->pos_x&0xff;
  mydata->msg.data[6]=(mydata->pos_x>>8)&0xff;
  mydata->msg.data[7]=mydata->pos_y&0xff;
  mydata->msg.data[8]=(mydata->pos_y>>8)&0xff;



  mydata->msg.crc = message_crc(&mydata->msg);

  mydata->message_lock = 0;
}

/* Process a received message at the front of the ring buffer.
 * Go through the list of neighbors. If the message is from a bot
 * already in the list, update the information, otherwise
 * add a new entry in the list
 */

void process_message () 
{
  uint8_t i;
  uint16_t ID;

  uint8_t d = estimate_distance(&RB_front().dist);
  //if (d > CUTOFF)
    //return;

  uint8_t *data = RB_front().msg.data;
  //if(data[4]==MOVE) return;
  ID = data[0] | (data[1] << 8);


  // search the neighbor list by ID
  for (i = 0; i < mydata->N_Neighbors; i++)
    if (mydata->neighbors[i].ID == ID) // found it
      break;

  if (i == mydata->N_Neighbors)   // this neighbor is not in list
    if (mydata->N_Neighbors < MAXN-1) // if we have too many neighbors, we overwrite the last entry
      mydata->N_Neighbors++;          // sloppy but better than overflow

  // i now points to where this message should be stored
  mydata->neighbors[i].ID = ID;
  mydata->neighbors[i].timestamp = kilo_ticks;
  mydata->neighbors[i].dist = d;
  mydata->neighbors[i].localized = data[2];
  mydata->neighbors[i].gradient = data[3];
  mydata->neighbors[i].n_bot_state = data[4];
  /*int tmp5=(int)(int8_t)data[5];
  float tmp6=(float)data[6]/100;
  if(tmp5<0) tmp6*=-1;
  int tmp7=(int)(int8_t)data[7];
  float tmp8=(float)data[8]/100;
  if(tmp7<0) tmp8*=-1;
  mydata->neighbors[i].pos_x=tmp5+tmp6;
  mydata->neighbors[i].pos_y=tmp7+tmp8;*/
  mydata->neighbors[i].pos_x=data[5]|data[6]<<8;
  mydata->neighbors[i].pos_y=data[7]|data[8]<<8;

}

/* Go through the list of neighbors, remove entries older than a threshold,
 * currently 2 seconds.
 */
void purgeNeighbors(void)
{
  int8_t i;

  for (i = mydata->N_Neighbors-1; i >= 0; i--) {
        

    if (kilo_ticks - mydata->neighbors[i].timestamp  > 32) //32 ticks = 1 s
      { //this one is too old. 
      if(kilo_uid==198) printf("expire!!!!!!!!!!!!!!!!:%d",mydata->neighbors[i].ID);
	mydata->neighbors[i] = mydata->neighbors[mydata->N_Neighbors-1]; //replace it by the last entry
	mydata->N_Neighbors--;
      }
      
  }
}

void set_motion(int new_motion)
{
    // Only take an action if the motion is being changed.
    int current_motion = get_move_type();
    if (current_motion != new_motion)
    {
        current_motion = new_motion;
        set_move_type(new_motion);

        if (current_motion == STOP)
        {
            set_motors(0, 0);
        }
        else if (current_motion == FORWARD)
        {
            spinup_motors();
            set_motors(kilo_straight_left, kilo_straight_right);
        }
        else if (current_motion == LEFT)
        {
            spinup_motors();
            set_motors(kilo_turn_left, 0);
        }
        else if (current_motion == RIGHT)
        {
            spinup_motors();
            set_motors(0, kilo_turn_right);
        }
    }
}

uint8_t find_nearest_N_dist()
{
  uint8_t i,index=-1;
  uint8_t dist = UINT8_MAX;

  for(i = 0; i < mydata->N_Neighbors; i++)
  {
    if(mydata->neighbors[i].n_bot_state==MOVE_OUTSIDE||mydata->neighbors[i].n_bot_state==MOVE_INSIDE) continue;
    if(mydata->neighbors[i].dist < dist)
	  {
	    dist = mydata->neighbors[i].dist;
      index=i;
	  }
  }
    //if(kilo_uid==13) printf("nearest:%d\n",mydata->neighbors[i-1].ID);
  if(mydata->neighbors[index].ID==0) mydata->accurate=1;
  return dist;
}

uint8_t find_nearest_N_gra()
{
  uint8_t i,index=-1;
  uint8_t dist = UINT8_MAX;

  for(i = 0; i < mydata->N_Neighbors; i++)
  {
    if(mydata->neighbors[i].n_bot_state==MOVE_OUTSIDE||mydata->neighbors[i].n_bot_state==MOVE_INSIDE) continue;
    if(mydata->neighbors[i].dist < dist)
	  {
	    dist = mydata->neighbors[i].dist;
      index=i;
	  }
  }
    //if(kilo_uid==13) printf("nearest:%d\n",mydata->neighbors[i-1].ID);
  return index==-1?0:mydata->neighbors[index].gradient;
}

int neigh_moving()
{
  uint8_t i;
  for (i = 0; i < mydata->N_Neighbors; i++){
    if(mydata->neighbors[i].n_bot_state==MOVE_INSIDE||mydata->neighbors[i].n_bot_state==MOVE_OUTSIDE) {
      return 1;
    }
  }
  return 0;
}

int is_edge_bot()
{
    int i;
    int max=-1;
    int maxID=-1;
    //if(get_bot_state()==MOVE)
      //  return 1;
    for(i=0;i<mydata->N_Neighbors;i++)
    {
        if(mydata->neighbors[i].n_bot_state!=JOINED_SHAPE&&mydata->neighbors[i].dist<CUTOFF&&mydata->neighbors[i].gradient>=max){
            max=mydata->neighbors[i].gradient;
            if(mydata->neighbors[i].ID>maxID)
                maxID=mydata->neighbors[i].ID;
        }
    }
    //if(kilo_uid==40) printf("amxId:%d\n",maxID);
    if(mydata->gradient_value > max) return 1;
    else if(mydata->gradient_value == max)
        if(kilo_uid > maxID) return 1;
    return 0;
}

int in_shape(){
  /*uint8_t i,index=-1;
  uint8_t dist = UINT8_MAX;

  for(i = 0; i < mydata->N_Neighbors; i++)
  {
    if(mydata->neighbors[i].n_bot_state==MOVE_OUTSIDE||mydata->neighbors[i].n_bot_state==MOVE_INSIDE) continue;
    if(mydata->neighbors[i].dist < dist)
	  {
	    dist = mydata->neighbors[i].dist;
      index=i;
	  }
  }
  if(mydata->neighbors[index].n_bot_state==JOINED_SHAPE) return 1;
  return 0;*/
  //if(mydata->pos_x+mydata->bmp_x<0||mydata->pos_x+mydata->bmp_x>255||mydata->bmp_y-mydata->pos_y<0||mydata->bmp_y-mydata->pos_y>255)return 0;
  int posx,posy,size=2;
  posx=mydata->pos_x/size+mydata->bmp_x;
  posy=mydata->bmp_y-mydata->pos_y/size;
  if(mydata->pos_x%size>=size/2&&size!=1) posx++;
  if(mydata->pos_y%size>=size/2&&size!=1) posy--;
    //posx=mydata->bmp_x+mydata->pos_x;
    //posy=mydata->bmp_y-mydata->pos_y;
    if(posx<0||posx>255||posy<0||posy>255)return 0;
  if(mydata->accurate&&!mydata->bmpPixel[posy][posx]) return 1;
  //if(mydata->accurate&&mydata->pos_x>0&&mydata->pos_x<240&&mydata->pos_y>0&&mydata->pos_y<240) return 1;
  return 0;
}

void follow_edge(uint8_t cur)
{
  //printf("!!!!!!!!!!!!!!%d %d \n",mydata->prev,cur);
  if(cur < DESIRED_DISTANCE)
  {
        if(mydata->prev < cur)
            set_motion(FORWARD);
        else
            set_motion(LEFT);
  }
  else
  {
      if(mydata->prev > cur)
        set_motion(FORWARD);
    else
        set_motion(RIGHT);
  }
  mydata->prev = cur;

}

/*double *trilateration(double x1,double y1,double d1, double x2, double y2,double d2, double x3, double y3, double d3)
    {
        static double d[2]={0.0,0.0};
        double a11 = 2*(x1-x3);
        double a12 = 2*(y1-y3);
        double b1 = (x1*x1)-(x3*x3) +(y1*y1)-(y3*y3) +(d3*d3)-(d1*d1);
        double a21 = 2*(x2-x3);
        double a22 = 2*(y2-y3);
        double b2 = (x2*x2)-(x3*x3) +(y2*y2)-(y3*y3) +(d3*d3)-(d2*d2);
        d[0]=(b1*a22-a12*b2)/(a11*a22-a12*a21);
        d[1]=(a11*b2-b1*a21)/(a11*a22-a12*a21);
        return d;
    }*/

void localization()
{

  int nlist[10];
	int num=0;
	int i;
	for(i=0;i<mydata->N_Neighbors;i++)
	{
    if(mydata->neighbors[i].pos_x==0&&mydata->neighbors[i].pos_y==0) {
      //printf("!!!!!!!%d\n",mydata->neighbors[i].ID);
      continue;
    }
		if(mydata->neighbors[i].localized==1&&(mydata->neighbors[i].n_bot_state==JOINED_SHAPE))
		{
			nlist[num++]=i;
			if(num>9) break;
		}
	}
	if(num>3)
	{
		//after 100s the pos maybe right
    /*if(mydata->timer>=0&&mydata->timer<1000){
      mydata->timer++;
    }*/
    /*int l1=nlist[0];
    int l2=nlist[1];
    int l3=nlist[2];
    int l4=nlist[3];
    int x1=mydata->neighbors[l1].pos_x;
    int y1=mydata->neighbors[l1].pos_y;
    int x2=mydata->neighbors[l2].pos_x;
    int y2=mydata->neighbors[l2].pos_y;
    int x3=mydata->neighbors[l3].pos_x;
    int y3=mydata->neighbors[l3].pos_y;
    int x4=mydata->neighbors[l4].pos_x;
    int y4=mydata->neighbors[l4].pos_y;
    MY_location_Init(x1,y1,x2,y2,x3,y3,x4,y4);
    double *d;
    d=Printf(mydata->neighbors[l1].dist,mydata->neighbors[l2].dist,mydata->neighbors[l3].dist,mydata->neighbors[l4].dist);
    */
    //mydata->accurate=0;
    float tmp_x=mydata->pos_x;
    float tmp_y=mydata->pos_y;
    int it=0,maxdiff=0;
    while(1){
    for(i=0;i<num;i++)
		{
			maxdiff=0;
      int l=nlist[i];
			float pos_x=mydata->neighbors[l].pos_x;
			float pos_y=mydata->neighbors[l].pos_y;
			float c=sqrt((tmp_x-pos_x)*(tmp_x-pos_x)+(tmp_y-pos_y)*(tmp_y-pos_y));
			float v_x=c==0?0:(tmp_x-pos_x)/c;
			float v_y=c==0?0:(tmp_y-pos_y)/c;
			float n_x=pos_x+mydata->neighbors[l].dist*v_x;
			float n_y=pos_y+mydata->neighbors[l].dist*v_y;
			tmp_x-=(tmp_x-n_x)/4.0;
			tmp_y-=(tmp_y-n_y)/4.0;
      it++;
      if(abs(sqrt((tmp_x-pos_x)*(tmp_x-pos_x)+(tmp_y-pos_y)*(tmp_y-pos_y))-mydata->neighbors[l].dist)>maxdiff){
        maxdiff=abs(sqrt((tmp_x-pos_x)*(tmp_x-pos_x)+(tmp_y-pos_y)*(tmp_y-pos_y))-mydata->neighbors[l].dist);
      }
      //if((sqrt((tmp_x-pos_x)*(tmp_x-pos_x)+(tmp_y-pos_y)*(tmp_y-pos_y))-mydata->neighbors[l].dist)>maxdiff)
        //maxdiff=abs(sqrt((tmp_x-pos_x)*(tmp_x-pos_x)+(tmp_y-pos_y)*(tmp_y-pos_y))-mydata->neighbors[l].dist);
      //if(kilo_uid==4)
      //printf("distance:%d %d %d %d\n",abs(sqrt((tmp_x-pos_x)*(tmp_x-pos_x)+(tmp_y-pos_y)*(tmp_y-pos_y))-mydata->neighbors[l].dist),kilo_uid,mydata->pos_x,mydata->pos_y);
		}
    if(it>30) break;
    if(maxdiff<1) break;
    }
    /*if(maxdiff<2){
      mydata->pos_x=(int16_t)tmp_x;
      mydata->pos_y=(int16_t)tmp_y;
      mydata->accurate=1;
    }*/
    mydata->pos_x=(int16_t)tmp_x;
    mydata->pos_y=(int16_t)tmp_y;
    
    //int16_t newx=(int16_t)*(d+0);
    //int16_t newy=(int16_t)*(d+1);
    //if(abs(mydata->pos_x-newx)+abs(mydata->pos_y-newy)>0&&abs(mydata->pos_x-newx)+abs(mydata->pos_y-newy)<50||mydata->pos_x==0&&mydata->pos_y==0){
      //mydata->pos_x=(int16_t)*(d+0);
      //mydata->pos_y=(int16_t)*(d+1);
    
    //if(pre_x==mydata->pos_x&&pre_y==mydata->pos_y)
    //mydata->timer=0;
		  setup_message();
    //}
          if(kilo_uid==43) printf("id:%d x=%d  y=%d \n",kilo_uid,mydata->pos_x,mydata->pos_y);
      //if(kilo_uid==8&&mydata->pos_x>0&&mydata->pos_y>0) printf("%d %d %d %d %d %d %d %d %d\n",mydata->neighbors[l1].pos_x,mydata->neighbors[l1].pos_y,mydata->neighbors[l1].dist,mydata->neighbors[l2].pos_x,mydata->neighbors[l2].pos_y,mydata->neighbors[l2].dist,mydata->neighbors[l3].pos_x,mydata->neighbors[l3].pos_y,mydata->neighbors[l3].dist);

	}
}

void gradient_formation()
{
  uint8_t i;
  uint8_t  new_g;
  if(kilo_uid==0) {
    new_g=0;
  }
  else
  {
    new_g=UINT8_MAX;
    for(i=0;i<mydata->N_Neighbors;i++)
    {
      //if(kilo_uid==118) printf("idididididi:%d ",mydata->N_Neighbors);
      if(mydata->neighbors[i].ID>3&&mydata->neighbors[i].n_bot_state==JOINED_SHAPE&&get_bot_state()==WAIT_TO_MOVE) continue;
      if(mydata->neighbors[i].dist<CUTOFF&&mydata->neighbors[i].n_bot_state!=MOVE_INSIDE&&mydata->neighbors[i].n_bot_state!=MOVE_OUTSIDE)
      {
        if(new_g>mydata->neighbors[i].gradient)
        {

          new_g=mydata->neighbors[i].gradient;
        }
      }
    }
    if(new_g!=UINT8_MAX){
      new_g++;
    }
    //if(kilo_uid==1)printf("!!!!!!!!!!!!%d:\n",mydata->gradient_value);
  }
  if(new_g!=mydata->gradient_value){
    mydata->gradient_value=new_g;
    setup_message();
  }
  /*uint8_t min = UINT8_MAX-1;
    for (i = 0; i < mydata->N_Neighbors; i++){
	  if(mydata->neighbors[i].n_bot_state==MOVE||mydata->neighbors[i].dist>CUTOFF) 
      continue;
    if (mydata->neighbors[i].gradient < min)
      min = mydata->neighbors[i].gradient;
    }

  if (kilo_uid == 0)  
        min = 0;
  
  if (mydata->gradient_value != min+1)
    {
      mydata->gradient_value = min+1;
      setup_message();
    }
  */
  if(get_bot_state()==MOVE_INSIDE) set_color(colors[9]);
  else
    set_color(colors[mydata->gradient_value%8+1]);

}

int eachBit(int byte,int bit)
{
	return (byte&(int)pow(2,bit))>>(bit);
}

void bmpDataPart(FILE* fpbmp)
{
     unsigned int OffSet = 0;    // OffSet from Header part to Data Part
     long BmpWidth = 0;          // The Width of the Data Part
     long BmpHeight = 0;         // The Height of the Data Part

     fseek(fpbmp, 10L, SEEK_SET);
     fread(&OffSet, sizeof(char), 4, fpbmp);   
     fseek(fpbmp, 18L, SEEK_SET);
     fread(&BmpWidth, sizeof(char), 4, fpbmp);
     fread(&BmpHeight, sizeof(char), 4, fpbmp);

     int i, j;
     int lineByte = ((BmpWidth*1+31)/8 )/4*4;
     //unsigned char bmpPixel[BmpHeight][lineByte*8];
     unsigned char* bmpPixelTmp = NULL;
     FILE* fpDataBmp;

     /* New a file to save the data matrix */
     if((fpDataBmp=fopen("bmpData.dat","w+")) == NULL)
     {
      fprintf(stderr, "Failed to construct file bmpData.dat.!!!");
      exit(1);
     }
     fseek(fpbmp, OffSet, SEEK_SET);
     if ((bmpPixelTmp=(unsigned char*)malloc(sizeof(char)*lineByte*BmpHeight))==NULL)
     {
      fprintf(stderr, "Data allocation failed.!!!\n");
      exit(1);
     }
     fread(bmpPixelTmp, sizeof(char), lineByte*BmpHeight, fpbmp);
     /* Read the data to Matrix and save it in file bmpData.dat */
     int cnt=0;
     for(i =0; i < BmpHeight; i++)
     {
      fprintf(fpDataBmp, "The data in line %-3d:\n", i+1);
      
      for(j = 0; j < lineByte; j++)
      {
        	int k;
			for(k=0;k<8;k++){
				mydata->bmpPixel[i][8*j+k] = eachBit(bmpPixelTmp[lineByte*(BmpHeight-1-i)+j],7-k);
            	fprintf(fpDataBmp, "%d", mydata->bmpPixel[i][8*j+k]);
            }
      
	  }
     }
     free(bmpPixelTmp);
     fclose(fpDataBmp);
}

void setup() {
  //rand_seed(kilo_uid + 1); //seed the random number generator
	FILE *fpbmp = fopen("1.bmp", "r+");
     if (fpbmp == NULL)
     {
      fprintf(stderr, "Open lena.bmp failed!!!\n");
      return;
     }

     //bmpFileTest(fpbmp);                //Test the file is bmp file or not
     //bmpHeaderPartLength(fpbmp);        //Get the length of Header Part
     //BmpWidthHeight(fpbmp);             //Get the width and width of the Data Part
     //bmpFileHeader(fpbmp);            //Show the FileHeader Information
     //bmpInfoHeader(fpbmp);            //Show the InfoHeader Information
     bmpDataPart(fpbmp);                //Reserve the data to file

     fclose(fpbmp);
     int i,j,k=0;
     for(i=255;i>=0;i--){
       if(k) break;
       for(j=0;j<256;j++)
       if(mydata->bmpPixel[i][j]==0){
         mydata->bmp_y=i;
         mydata->bmp_x=j;
         k=1;
         break;
       }
     }
printf("???????????%d %d\n",mydata->bmp_x,mydata->bmp_y);
printf("!!!!!!!!!!!!!%d \n",mydata->bmpPixel[18+31][205-121]);

  mydata->message_lock = 0;
  mydata->gradient_value = UINT8_MAX;
  mydata->N_Neighbors = 0;
  mydata->prev = UINT8_MAX;
  mydata->pos_x=0;
  mydata->pos_y=0;
  mydata->localized=0;
  mydata->accurate=0;

  set_move_type(STOP);
  set_bot_state(START);
  mydata->bot_type=1;
  if(kilo_uid<4) {
    mydata->bot_type=0;
    mydata->gradient_value=1;
    mydata->localized=1;
  }


  //setup_message();
  // the special root bot originally had UID 10000, we change it to 0
  if (kilo_uid == 0)  {
    mydata->gradient_value = 0;
    mydata->pos_x=-20;
    mydata->pos_y=0;

  }
  if(kilo_uid==1){
    mydata->pos_x=20;
    mydata->pos_y=0;
  }
  if(kilo_uid==2){
    mydata->pos_x=0;
    mydata->pos_y=-35;
  }
  if(kilo_uid==3){
    mydata->pos_x=0;
    mydata->pos_y=35;
  }

  setup_message();

}

void loop()
{
  purgeNeighbors();
  //process messages in the RX ring buffer
  while (!RB_empty()) {
    process_message();
    RB_popfront();
  }

  if(get_bot_state()==START){
    if(mydata->bot_type==0){
      set_bot_state(JOINED_SHAPE);
      setup_message();
    }
    else{
      gradient_formation();
      localization();
      //printf("localiz:%d %f %f %d\n",kilo_uid,mydata->pos_x,mydata->pos_y,kilo_ticks);
      if(kilo_ticks>160){
        set_bot_state(WAIT_TO_MOVE);
        setup_message();
      }
    }
  }
  else if(get_bot_state()==WAIT_TO_MOVE){
    gradient_formation();
    localization();
    if(!neigh_moving()){
      if(is_edge_bot()){
        if(mydata->timer<32) mydata->timer++;
        if(mydata->timer==32){
          set_bot_state(MOVE_OUTSIDE);
          setup_message();
        }
        set_motion(RIGHT);
        mydata->prev=find_nearest_N_dist();

      }
    }
  }
  else if(get_bot_state()==MOVE_OUTSIDE){
    gradient_formation();
    localization();
    if(in_shape()){
      printf("join %d %d %d\n",kilo_uid,mydata->pos_x,mydata->pos_y);
      set_bot_state(MOVE_INSIDE);
      setup_message();
    }
    uint8_t cur=find_nearest_N_dist();
    if(cur!=mydata->prev){
      follow_edge(cur);
    }
  }
  else if(get_bot_state()==MOVE_INSIDE){
    //set_color(colors[8]);
    gradient_formation();
    localization();
    if(!in_shape()){
      set_bot_state(JOINED_SHAPE);
          printf("im:%d here!!!!%d %d  11111111111\n",kilo_uid,mydata->pos_x,mydata->pos_y);

      setup_message();
    }
    if(mydata->gradient_value<=find_nearest_N_gra()){
      set_bot_state(JOINED_SHAPE);
          printf("im:%d here!!!!%d %d  22222222222\n",kilo_uid,mydata->pos_x,mydata->pos_y);

      setup_message();
    }
    uint8_t cur=find_nearest_N_dist();
    if(cur!=mydata->prev){
      follow_edge(cur);
    }
  }
  else if(get_bot_state()==JOINED_SHAPE){
    set_color(colors[0]);
    mydata->localized=1;
    set_motion(STOP);
    setup_message();
  }
/*
  gradient_formation();
  localization();
        if(mydata->timer>=1&&mydata->timer<60)
      {
        mydata->timer++;
        //if(kilo_uid==19)printf("???????????????????%d\n",mydata->N_Neighbors);
        return;
      }
      mydata->timer=0;
  if(kilo_ticks>200)
  if(is_edge_bot()&&!neigh_moving()) {
    if(get_bot_state()==LISTEN){
      set_motion(RIGHT);
      mydata->prev=find_nearest_N_dist();
      mydata->timer=1;
      set_bot_state(MOVE);
      setup_message();
      
    }
    uint8_t cur=find_nearest_N_dist();
      //printf("!loooooooooooooooooop%d %d \n",mydata->prev,cur);

    if(cur!=mydata->prev){

      follow_edge(cur);
    }
  //mydata->timer++;
  //printf("hhhtimer:%d kilotick:%d\n",mydata->timer,kilo_ticks);
  }*/
}



int main()
{
    // initialize hardware
    kilo_init();
    // initialize ring buffer
    RB_init();
    // register message callbacks
    kilo_message_rx = rxbuffer_push; 
    kilo_message_tx = message_tx;   
    // register your program
    kilo_start(setup, loop);    
    return 0;
}
