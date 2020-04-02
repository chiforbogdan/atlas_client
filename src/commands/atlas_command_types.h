#ifndef __ATLAS_COMMAND_TYPE_H__
#define __ATLAS_COMMAND_TYPE_H__

typedef enum _atlas_cmd_type
{
    /* Register command: payload is the client identity*/
    ATLAS_CMD_REGISTER = 0,

    /* Keepalive command: payload is a 2 byte token*/
    ATLAS_CMD_KEEPALIVE,

    /* Identity command: payload is the client identity */
    ATLAS_CMD_IDENTITY,

    /* IP command: payload is the local IP address used by gateway to connect to the client*/
    ATLAS_CMD_IP_PORT,

    /* Telemetry hostname command: payload is client hostname */
    ATLAS_CMD_TELEMETRY_HOSTNAME = 500,

    /* Telemetry kernel info command: payload is client kernel info */
    ATLAS_CMD_TELEMETRY_KERN_INFO,
    
    /* Telemetry sysinfo uptime command: payload is number of seconds since boot */
    ATLAS_CMD_TELEMETRY_SYSINFO_UPTIME,

    /* Telemetry sysinfo total ram command: payload is total ram */
    ATLAS_CMD_TELEMETRY_SYSINFO_TOTALRAM,

    /* Telemetry sysinfo free ram command: payload is free ram */
    ATLAS_CMD_TELEMETRY_SYSINFO_FREERAM,

    /* Telemetry sysinfo shared ram command: payload is shared ram */
    ATLAS_CMD_TELEMETRY_SYSINFO_SHAREDRAM,

    /* Telemetry sysinfo buffer ram command: payload is buffer ram */
    ATLAS_CMD_TELEMETRY_SYSINFO_BUFFERRAM,

    /* Telemetry sysinfo total swap command: payload is total swap */
    ATLAS_CMD_TELEMETRY_SYSINFO_TOTALSWAP,

    /* Telemetry sysinfo free swap command: payload is free swap */
    ATLAS_CMD_TELEMETRY_SYSINFO_FREESWAP,

    /* Telemetry sysinfo procs command: payload is number of processes */
    ATLAS_CMD_TELEMETRY_SYSINFO_PROCS,

    /* Telemetry sysinfo load 1 command: payload is 1 minute load average */
    ATLAS_CMD_TELEMETRY_SYSINFO_LOAD1,

    /* Telemetry sysinfo load 5 command: payload is 5 minutes load average */
    ATLAS_CMD_TELEMETRY_SYSINFO_LOAD5,

    /* Telemetry sysinfo load 15 command: payload is 15 minutes load average */
    ATLAS_CMD_TELEMETRY_SYSINFO_LOAD15,

    /* Telemetry alert external push rate: payload is a number of seconds indicating
    an interval at which data will be pushed to gateway */
    ATLAS_CMD_TELEMETRY_ALERT_EXT_PUSH_RATE,

    /* Telemetry alert internal scan rate: payload is a number of seconds indicating
    an interval at which data will be scanned internally by the client and pushed to
    gateway only if passes the a given threshold */ 
    ATLAS_CMD_TELEMETRY_ALERT_INT_SCAN_RATE,

    /* Telemetry alert threshold value: payload is a string indicating the threshold
    value. This string will be parsed by each telemetry feature (application specific) */
    ATLAS_CMD_TELEMETRY_ALERT_THRESHOLD,
    
    /* Telemetry packets per minute command: payload is the number of packets received in 1 minute */
    ATLAS_CMD_TELEMETRY_PACKETS_PER_MINUTE,
    
    /* Telemetry packets average command: payload is the average length of received packets in 1 minute */
    ATLAS_CMD_TELEMETRY_PACKETS_AVG,


    /* Command types shared with the data plane agent */

    /* Policy command: payload is ATLAS_CMD_DATA_PLANE_POLICY_QOS,
     * ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE,
     * ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_MAXLEN */
    ATLAS_CMD_DATA_PLANE_POLICY = 1000,

    /* Packets per minute command: payload is the number of received packets per minute */
    ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE,
    
    /* Packets average command: payload is the average length of received packets*/
    ATLAS_CMD_DATA_PLANE_PACKETS_AVG,

    /* Get identity comman: payload is empty */
    ATLAS_CMD_DATA_PLANE_GET_IDENTITY,
    
    /* Identity command: payload is a string containing the device identity */
    ATLAS_CMD_DATA_PLANE_IDENTITY,
    
    /* Command types shared with the data plane agent, client and gateway */

    /* Policy command: payload is the policy qos value */
    ATLAS_CMD_DATA_PLANE_POLICY_QOS = 1100,
    
    /* Policy command: payload is the policy packets_per_minute value */
    ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE,
    
    /* Policy command: payload is the policy packets_max_length value */
    ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_MAXLEN,
 
    /* Feature reputation request command: payload is the feature for which the data_plane 
     * requests the reputation value*/
    ATLAS_CMD_DATA_PLANE_FEATURE_REPUTATION = 1500,

    /* Feedback command: payload is ATLAS_CMD_DATA_PLANE_FEEDBACK_IDENTITY,
     * ATLAS_CMD_DATA_PLANE_FEEDBACK_SENSOR_TYPE,
     * ATLAS_CMD_DATA_PLANE_FEEDBACK_VALUE 
     * ATLAS_CMD_DATA_PLANE_FEEDBACK_RESPONSE_TIME*/
    ATLAS_CMD_DATA_PLANE_FEEDBACK,
   
    /* Feedback identity: payload is the identity for which the feedback is delivered */
    ATLAS_CMD_DATA_PLANE_FEEDBACK_IDENTITY,
    
    /* Sensor type command: payload is the sensor type */
    ATLAS_CMD_DATA_PLANE_FEEDBACK_SENSOR_TYPE,

    /* Sensor feedback command: payload is the feedback score */
    ATLAS_CMD_DATA_PLANE_FEEDBACK_SENSOR,

    /* Response time command: payload is the feedback score */
    ATLAS_CMD_DATA_PLANE_FEEDBACK_RESPONSE_TIME,

    // TODO rename remove this....
    ATLAS_CMD_DATA_PLANE_FEATURE_ERROR,
} atlas_cmd_type_t;

#endif /* __ATLAS_COMMAND_TYPE_H__ */
