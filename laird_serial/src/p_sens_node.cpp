/*
 * serial_node.cpp
 *
 *  Created on: 25.09.2017
 *      Author: mario
 */


#include <ros/ros.h>
#include <serial/serial.h>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Char.h>
#include <iostream>
#include <string>
#include <sstream>
#include <pressure_msgs/ble_pressure.h>

serial::Serial ser;
pressure_msgs::ble_pressure e;

//void write_callback(const std_msgs::String::ConstPtr& msg){
  //  ROS_INFO_STREAM("Writing to serial port" << msg->data);
  //  ser.write(msg->data);
//}

int main (int argc, char** argv){
    ros::init(argc, argv, "BT900_US_communication");
    ros::NodeHandle nh("~");
    ros::NodeHandle n;
    ros::Publisher pressure_pub = n.advertise<pressure_msgs::ble_pressure>("pressure",100);

    int state = 0;
    int cnHndl = 0;
    int cnHndl_1 = 0;
    int cnHndl_2 = 0;
    int atHndl = 0;
    int sensors = 0;
    std::string result, temp, outstr;
    std::string substring[10];
    std::string keywords_in[7];
    std::string keywords_out[8];

    keywords_in[0] = "error";
    keywords_in[1] = "fail";
    keywords_in[2] = "connection";
    keywords_in[3] = "write";
    keywords_in[4] = "read";
    keywords_in[5] = "sensors";
    keywords_in[6] = "exit";

    keywords_out[0] = "connect ";
    keywords_out[1] = "disconnect ";
    keywords_out[2] = "read ";
    keywords_out[3] = "pread ";
    keywords_out[4] = "stop ";
    keywords_out[5] = "write ";
    keywords_out[6] = "exit ";
    keywords_out[7] = "sensors ";

    std::string port = "/dev/ttyUSB0";
    nh.getParam("port", port);

    int baudrate = 115200;
    nh.getParam("baudrate", baudrate);

    std::string frame_id = "p_sen_com";
    nh.getParam("frame_id", frame_id);

    int n_sensor = 2;
    nh.getParam("n_sensor", n_sensor);

    std::string sensor_1 = "01F2EB685E2BE7";
    nh.getParam("sensor_1", sensor_1);

    std::string sensor_2 = "01F7B34480DDC8";
    nh.getParam("sensor_2", sensor_2);

    try
    {
        ser.setPort(port);
        ser.setBaudrate(baudrate);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    }
    catch (serial::IOException& e)
    {
        ROS_ERROR_STREAM("Unable to open port "<<port);
        return -1;
    }

    if(ser.isOpen()){
        ROS_INFO_STREAM("Serial Port initialized with baudrate "<<baudrate);
    }else{
        return -1;
    }

    ser.write(keywords_out[7]);

    ros::Rate loop_rate(10);

    while(ros::ok()){

    	ros::spinOnce();

        if(ser.available()){
            result = "";
            result = ser.read(ser.available());
            temp = result;
            std::size_t pos =  temp.find(" ");
            int i = 0;
            while(!temp.empty()){
            	substring[i] = temp.substr(0,pos);
            	temp = temp.substr(pos+1);
            	pos =  temp.find(" ");
            	i++;
            	if(pos>=temp.length()){
            		pos =  temp.find("\n");
            		substring[i] = temp.substr(0,pos);
            		break;
            	}
            }
            
            // keywords_in[5] = sensors
            // "sensors 0"
            // "sensors 1 conHndl_1"
            // "sensors 2 conHndl_1 conHndl_2"
            if (substring[0].compare(keywords_in[5])==0){
            	std::stringstream stream1(substring[1]);
            	if(sscanf(substring[1].c_str(),"%d",&sensors)!=1){}
            	if (sensors==0){
					ser.write(keywords_out[0]+sensor_1);
					state = 0;
            	}
            	else if(sensors==1){
					std::stringstream stream1(substring[2]);
					if(sscanf(substring[2].c_str(),"%d",&cnHndl_1)!=1){}
					if(n_sensor==1){
						ser.write(keywords_out[3]+"32");
						state = 2;
					}
					else{
						ser.write(keywords_out[0]+sensor_2);
						state = 1;
					}
            	}
            	else if (sensors==2){
					std::stringstream stream2(substring[2]);
					if(sscanf(substring[2].c_str(),"%d",&cnHndl_1)!=1){}
					std::stringstream stream3(substring[3]);
					if(sscanf(substring[3].c_str(),"%d",&cnHndl_2)!=1){}
					ser.write(keywords_out[3]+"32");
					state = 2;
            	}
            	else {
            		ROS_ERROR("Number of sensors out of range");
            	}
            }
            else{
				switch (state) {
					case 0:
						if (substring[0].compare(keywords_in[2])==0){
							ROS_INFO_STREAM("Connected to Sensor 1 with connection Handle "<<substring[1]);
							std::stringstream stream1(substring[1]);
							if(sscanf(substring[1].c_str(),"%d",&cnHndl_1)!=1){}
							if(n_sensor==1){
								ser.write(keywords_out[3]+"32");
								state = 2;
							}
							else{
								ser.write(keywords_out[0]+sensor_2);
								state = 1;
							}
						}
						else if(result.find(keywords_in[1])==0){
							ser.write(keywords_out[0]+sensor_1);
							ROS_INFO_STREAM("Sensor 1 not found, try again...");
						}
						else{
							ROS_INFO_STREAM("Read: " << result);
							ROS_INFO_STREAM("command:" << substring[0]);
						}
						break;
					case 1:
						if (substring[0].compare(keywords_in[2])==0){
							ROS_INFO_STREAM("Connected to Sensor 2 left with connection Handle "<<substring[1]);
							std::stringstream stream1(substring[1]);
							if(sscanf(substring[1].c_str(),"%d",&cnHndl_2)!=1){}
							ser.write(keywords_out[3]+"32");
							state = 2;
						}
						else if(result.find(keywords_in[1])==0){
							ser.write(keywords_out[0]+sensor_2);
							ROS_INFO_STREAM("Sensor 2 not found, try again...");
						}
						else{
							ROS_INFO_STREAM("Read: " << result);
							ROS_INFO_STREAM("command:" << substring[0]);
						}
						break;
					case 2:
						if (substring[0].compare(keywords_in[4])==0){
							if(sscanf(substring[1].c_str(),"%d",&cnHndl)!=1){}
							if(sscanf(substring[2].c_str(),"%d",&atHndl)!=1){}
							if (cnHndl== cnHndl_1){
								e.temp_right = ((float)strtoul(substring[3].c_str(),NULL,16)/100);
								e.press_right = ((float)strtoul(substring[4].c_str(),NULL,16)/1000000);
							}
							else if (cnHndl==cnHndl_2){
								e.temp_left = ((float)strtoul(substring[3].c_str(),NULL,16)/100);
								e.press_left = ((float)strtoul(substring[4].c_str(),NULL,16)/1000000);
							}
							e.header.stamp = ros::Time::now();
							e.header.frame_id = frame_id;
							pressure_pub.publish(e);
						}
						else if (substring[0].compare(keywords_in[2])==0){
							std::stringstream stream1(substring[1]);
							if(sscanf(substring[1].c_str(),"%d",&cnHndl)!=1){}
							if (cnHndl== cnHndl_1){
								ROS_WARN("Connected to Sensor 1 lost");
								cnHndl_1 = 0;
								e.temp_right = 0.0;
								e.press_right = 0.0;
							}
							else if (cnHndl==cnHndl_2){
								ROS_WARN("Connected to Sensor 2 lost");
								cnHndl_2 = 0;
								e.temp_left = 0.0;
								e.press_left = 0.0;
							}
						}
						else{
							//ROS_INFO_STREAM("command not detected State 1");
							//ROS_INFO_STREAM("Read: " << result);
							//ROS_INFO_STREAM("command:" << substring[0]);
						}
						break;
					default:
						ROS_INFO_STREAM("State error");
						break;
				}
			}
            ser.flush();
        }
        loop_rate.sleep();

    }
}