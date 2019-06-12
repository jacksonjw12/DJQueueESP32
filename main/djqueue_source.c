//Jackson Wheeler
//COEN 315 SCU
//Uses ESP ADF library
//ESP 32 Lyrat required
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "bluetoothDJQueue_service.h"
#include "mp3_decoder.h"
#include "http_stream.h"

#include "esp_http_client.h"
#include "json_utils.h"


static const char *TAG = "BLUETOOTH_SOURCE_EXAMPLE";

#define MAX_RESPONSE_BUFFER (2048)




//helper function for making GET requests
void getRequestJSON(char** returnTo, char* url, char* key){
    char *response_text = NULL;
    
    char *data_buf = NULL;

    esp_http_client_handle_t client = NULL;
    esp_http_client_config_t dj_config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };
    client = esp_http_client_init(&dj_config);
    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening connection");
        
        free(data_buf);
        
        esp_http_client_cleanup(client);
        return;
    }

    int data_length = esp_http_client_fetch_headers(client);
    if (data_length <= 0) {
        data_length = MAX_RESPONSE_BUFFER;
    }
    data_buf = malloc(data_length + 1);
    if(data_buf == NULL) {
        free(data_buf);
        esp_http_client_cleanup(client);
        return;
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);
    response_text = json_get_token_value(data_buf, key);
    if (response_text) {
        ESP_LOGI(TAG, "text = %s", response_text);
    }
    free(data_buf);
    
    esp_http_client_cleanup(client);
    *returnTo = response_text;
    return;
}
//forms a get request to get the current song url from the server
char *getCurrentSong(){
    char *current_song_uri = NULL;

    getRequestJSON(&current_song_uri, "http://play.jacksonwheelers.space/getCurrentSong","uri");

    ESP_LOGI(TAG, "JW[ song ] current song: %s", current_song_uri);
    return current_song_uri;
}

//useful for debugging/developement so kept in
void testWifi(){
    char *response_text = NULL;
    //char *uri = NULL;
    char *post_buffer = NULL;
    char *data_buf = NULL;
    esp_http_client_handle_t client = NULL;
    esp_http_client_config_t dj_config = {
        .url = "http://dj.jacksonwheelers.space/getInfo",
        .method = HTTP_METHOD_GET,
    };
    client = esp_http_client_init(&dj_config);
    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening connection");
        free(post_buffer);
        free(data_buf);
        
        esp_http_client_cleanup(client);
        return;
    }

    int data_length = esp_http_client_fetch_headers(client);
    if (data_length <= 0) {
        data_length = MAX_RESPONSE_BUFFER;
    }
    data_buf = malloc(data_length + 1);
    if(data_buf == NULL) {
        free(post_buffer);
        free(data_buf);
        esp_http_client_cleanup(client);
        return;
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);
    response_text = json_get_token_value(data_buf, "data");
    if (response_text) {
        ESP_LOGI(TAG, "text = %s", response_text);
    }
    free(post_buffer);
    free(data_buf);
    
    esp_http_client_cleanup(client);
}


void setup_wifi(esp_periph_set_handle_t set){

    ESP_LOGI(TAG, "JW[ 2 ] Setting up Wi-Fi network");

    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);
    ESP_LOGI(TAG, "JW[ 2.1 ] Finished setting up Wi-Fi network");
}


void app_main(void)
{
   

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    tcpip_adapter_init();

    ESP_LOGI(TAG, "JW [ 1 ] Starting DJ Queue");
    
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    // During debugging it is very useful to enable these log levels
    // esp_log_level_set("*", ESP_LOG_DEBUG);
    // esp_log_level_set("HTTP_STREAM", ESP_LOG_DEBUG);
    // esp_log_level_set("MP3_DECODER", ESP_LOG_DEBUG);
    // esp_log_level_set("STAGEFRIGHTMP3_DECODER", ESP_LOG_DEBUG);
    // esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_DEBUG);
    // esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_DEBUG);


    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    

    setup_wifi(set);

    

    ESP_LOGI(TAG, "JW [ 3 ] Create Bluetooth service");
    bluetooth_service_cfg_t bt_cfg = {
        .device_name = "ESP-ADF-SOURCE",
        .mode = BLUETOOTH_A2DP_SOURCE,
        .remote_name = CONFIG_BT_REMOTE_NAME,
    };
    bluetooth_service_start(&bt_cfg);
    ESP_LOGI(TAG, "JW [ 3.1 ] Finished Creating Bluetooth service");
    

    ESP_LOGI(TAG, "JW [4] Create Bluetooth peripheral");
    esp_periph_handle_t bt_periph = bluetooth_service_create_periph();

    ESP_LOGI(TAG, "JW [5] Start Bluetooth peripheral");
    esp_periph_start(set, bt_periph);
    
    audio_pipeline_handle_t pipeline;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    audio_element_handle_t bt_stream_writer;
    //pipeline elements
    audio_element_handle_t http_stream_reader, mp3_decoder;
    
    ESP_LOGI(TAG, "JW [ 6 ] Get Bluetooth stream");
    bt_stream_writer = bluetooth_service_create_stream();


    ESP_LOGI(TAG, "JW [ 7 ] Create http stream to read data");
    

    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_stream_reader = http_stream_init(&http_cfg);


    ESP_LOGI(TAG, "JW [ 8 ] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);


    audio_element_info_t mp3_info = {0};
    audio_element_getinfo(mp3_decoder, &mp3_info);

    

    ESP_LOGI(TAG, "JW [ 9 ] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, bt_stream_writer,   "bt");

    ESP_LOGI(TAG, "JW [ 10 ] Link it together http_stream-->mp3-->bt_stream_writer");
    audio_pipeline_link(pipeline, (const char *[]) {"http", "mp3", "bt"}, 3);


    ESP_LOGI(TAG, "JW [ 11 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    ESP_LOGI(TAG, "JW [ 12 ] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "JW [ 13 ] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);
    //OUTER LOOP

    int first = 1;
    int count = 0;
    while(1){
        count++;
         //pipeline main
        
        
        ESP_LOGE(TAG, "JW [14] BEGIN LOOP #%i",count);
        //sets current_song_uri
        ESP_LOGI(TAG, "JW [15] GETTING CURRENT SONG");
        ESP_LOGE(TAG, "");
        


        

        
        

        
        
        //for the first loop, play the ready sound
        if(first){
            first = 0;
            char* currentSong = "http://play.jacksonwheelers.space/ready.mp3";
            audio_element_set_uri(http_stream_reader, currentSong);
            ESP_LOGI(TAG, "JW [ 16 ] First Loop: Started audio_pipeline");
            audio_pipeline_run(pipeline);
        }
        else{
            // periph_bluetooth_play(bt_periph);

            char* currentSong = getCurrentSong();

            ESP_LOGI(TAG, "JW [ 17 ] setting song uri: %s",currentSong);

            audio_element_set_uri(http_stream_reader, currentSong);
            ESP_LOGI(TAG, "JW [ 18 ] Restarting audio_pipeline");
            audio_pipeline_run(pipeline);
            
        }
        
        char ** discovered;
        int taskIteration = 0;
        while(1){
            ESP_LOGI(TAG, "JW [ TL! ] Inner Task Listener Loop");
            
            // there is capability to get a list of found bt devices
            // the plan was to connect to a known wifi, and have the device chosen dynamically
            // there was not enough time for this
            // here is an example of getting the discovered devices
            // if(!taskIteration % 5){
            //     discovered = getDiscoveredDevices();
            //     int i;
            //     for(i = 0; i< getNumDiscoveredDevices(); i++){
                    
            //         ESP_LOGI(TAG, "JW [ BT ]Discovered: %s",discovered[i]);
                    
            //     }
            // }


            audio_event_iface_msg_t msg;

            esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
                continue;
            }


            //which pipeline element is the message from, or the bt peripheral
            ESP_LOGI(TAG, "http: %i , mp3: %i , btsw: %i , BT: %i",msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
                && msg.source == (void *) http_stream_reader,msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
                && msg.source == (void *) mp3_decoder,msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
                && msg.source == (void *) bt_stream_writer,
                msg.source_type == PERIPH_ID_BLUETOOTH && msg.source == (void *)bt_periph);
            //what is the message
            ESP_LOGI(TAG, "CMD: %i , DATA: %i", (int) msg.cmd , (int) msg.data);

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) http_stream_reader
                && msg.cmd == AEL_MSG_CMD_REPORT_CODEC_FMT && (int) msg.data == 0) {
                ESP_LOGW(TAG, "[ * ] Got http stream");
                
                continue;
            }

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
                && msg.source == (void *) mp3_decoder
                && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {

                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);




                ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);

                
                audio_element_setinfo(bt_stream_writer, &music_info);

                //wakes up bt
                audio_pipeline_resume(pipeline);


                continue;
            }

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) http_stream_reader
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == 15) {
                ESP_LOGW(TAG, "[ * ] Http stream interrupt, restart pipeline");
               
                break;
            }
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) http_stream_reader
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_ERROR_OPEN) {
                ESP_LOGW(TAG, "[ * ] Got an http error status open, restarting pipeline");
                
                break;
            }


            /* when the Bluetooth is disconnected or suspended, they will usually be reconnected, in future if this persists the device should restart or crash */
            if (msg.source_type == PERIPH_ID_BLUETOOTH
                    && msg.source == (void *)bt_periph) {

                if (msg.cmd == PERIPH_BLUETOOTH_DISCONNECTED) {
                    ESP_LOGW(TAG, "[ * ] Bluetooth disconnected");
                   

                }
                else if(msg.cmd == PERIPH_BLUETOOTH_AUDIO_SUSPENDED){
                    ESP_LOGW(TAG, "[ * ] Bluetooth audio suspended");
                    
                }
                continue;
            }

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) mp3_decoder
                 ) {
                ESP_LOGE(TAG, "[ * ] mp3 stream message %i", (int)msg.data);
                

                if((int)msg.data == 14){
                    ESP_LOGE(TAG, "[ * ] Would have restarted pipeline if this was enabled");
                    // it was necessary to use a break here in a previous version, the logic exists because it gives good error messages in debugging 
                    continue;

                }
                
                continue;
            }
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) bt_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
                ESP_LOGW(TAG, "[ * ] Stop event received");
               //should probably crash here, but because we break on http interrupts, this never happens
            }

            
            taskIteration++;

            
        }//inner loop

        //stop the pipeline
        audio_pipeline_stop(pipeline);
        audio_pipeline_wait_for_stop(pipeline);

        //reset the http stream reader, because it has reached an interrupt
        audio_element_reset_state(http_stream_reader);
        //by this time, the http stream reader has already passed on a stop event to the mp3 decoder
        //the decoder also has bad values relating to song length, and if not reset, it will think it is finished a send a stop event shortly into the new song
        audio_element_reset_state(mp3_decoder);


    }
    //this is never reached
    ESP_LOGI(TAG, "[ 10 ] Done with outer loop");

    ESP_LOGI(TAG, "[ 10.12 ] Debug ");
    bluetooth_service_destroy();
    ESP_LOGI(TAG, "[ 10.13 ] Debug ");
    esp_periph_set_destroy(set);
    ESP_LOGI(TAG, "[ 10.14 ] Debug ");

}
