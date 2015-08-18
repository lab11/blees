#include <string.h>

#include "app_util.h"
#include "ble_srv_common.h"
#include "nordic_common.h"

#include "ble_ess.h"

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_ess       Environmental Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
    p_ess->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_ess       Environmental Sensing Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_ess->conn_handle = BLE_CONN_HANDLE_INVALID;
}

static bool is_char_value_handle(ble_gatts_evt_hvc_t * p_hvc, ble_ess_t * p_ess){

    if ((p_hvc->handle == (p_ess->pres_char_handles.value_handle))||
        (p_hvc->handle == (p_ess->hum_char_handles.value_handle)) ||
        (p_hvc->handle == (p_ess->temp_char_handles.value_handle))||
        (p_hvc->handle == (p_ess->lux_char_handles.value_handle)) ||
        (p_hvc->handle == (p_ess->acc_char_handles.value_handle)))
    {

        return true;
    }
    return false;
}

/**@brief Function for handling the HVC event.
 *
 * @details Handles HVC events from the BLE stack.
 *
 * @param[in]   p_ess       ESS structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_hvc(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_hvc_t * p_hvc = &p_ble_evt->evt.gatts_evt.params.hvc;
    
    if (is_char_value_handle(p_hvc, p_ess))
    {
        ble_ess_evt_t evt;
        
        evt.evt_type = BLE_ESS_EVT_NOTIFICATION_CONFIRMED;
        p_ess->evt_handler(p_ess, &evt);
    }
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_ess       ESS structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_ess->pressure.trigger_handle)
    {
        memcpy(&(p_ess->temperature.trigger_val_cond), p_evt_write->data, 1);
        memcpy(p_ess->temperature.trigger_val_buff, p_evt_write->data, 4);
    }
    else if (p_evt_write->handle == p_ess->humidity.trigger_handle)
    {
        memcpy(&(p_ess->humidity.trigger_val_cond), p_evt_write->data, 1);
        memcpy(p_ess->humidity.trigger_val_buff, p_evt_write->data, 4);
    }
    else if (p_evt_write->handle == p_ess->temperature.trigger_handle)
    {
        memcpy(&(p_ess->temperature.trigger_val_cond), p_evt_write->data, 1);
        memcpy(p_ess->temperature.trigger_val_buff, p_evt_write->data, 4);
    }
    else if (p_evt_write->handle == p_ess->lux.trigger_handle)
    {
        memcpy(&(p_ess->lux.trigger_val_cond), p_evt_write->data, 1);
        memcpy(p_ess->lux.trigger_val_buff, p_evt_write->data, 4);
    }
    else if (p_evt_write->handle == p_ess->acceleration.trigger_handle)
    {
        memcpy(&(p_ess->acceleration.trigger_val_cond), p_evt_write->data, 1);
        memcpy(p_ess->acceleration.trigger_val_buff, p_evt_write->data, 4);
    }
}



void ble_ess_on_ble_evt(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_ess, p_ble_evt);
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_ess, p_ble_evt);
            break;
            
        case BLE_GATTS_EVT_HVC:
            on_hvc(p_ess, p_ble_evt);
            break;
        case BLE_GATTS_EVT_WRITE:
            //break;
            on_write(p_ess, p_ble_evt);
            break;
        default:
            break;
    }
}


/**@brief Function for adding an ESS characteristic.
 *
 * @param[in]   p_ess        Environmental Service structure.
 * @param[in]   p_ess_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
/* Every Characteristc consists of at least 2 attributes:
 *   1) Characteristic Declaration Attribute
 *       which consists of 3 sub-properties:
 *          a) The Characteristic Properties
 *          b) The Characteristic Value Handle
 *          c) The Characteristc UUID
 *
 *   2) Characteristic Value Attribute
 *
 *   And sometimes a third attribute:
 *   3) Descriptors Attribute
 *       - If used, this must be placed after the characteristc value attribute
 *
 *   These 2 (or 3) attributes together make up the characteristic description
 */
static uint32_t ess_char_add(ble_ess_t * p_ess,
                            const ble_ess_init_t * p_ess_init,
                            int ESS_CHAR_UUID,
                            uint8_t uuid_type,
                            ess_char_data_t * char_data,
                            uint8_t * fake_data_p,
                            uint16_t init_char_len,
                            uint16_t max_char_len,
                            ess_char_trigger_init_data_t * char_trigger_init_data,
                            uint8_t * trigger_val,
                            ble_gatts_char_handles_t * char_handles)
{
    
    uint32_t err_code;
    
    ble_gatts_char_md_t char_md; // char metadata (part of Characteristic Properties)
    ble_gatts_attr_t    attr_char_value; //Characteristic Value Attribute- the actual data!
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md; // attribute metadata
    ble_gatts_attr_md_t cccd_md;
    
    memset(&char_md, 0, sizeof(char_md));
    
    /***** set characteristic properties *****/
    char_md.char_props.read   = 1; // mandatory
    char_md.char_props.notify = p_ess_init->is_notify_supported; // optional
    p_ess->is_notify_supported = char_md.char_props.notify;
    
    /***** external properties are optional ****/
    char_md.char_ext_props.reliable_wr = 1;
    char_md.char_ext_props.wr_aux = 1;
    
    
    /******** if notify is enabled ******/
    memset(&cccd_md, 0, sizeof(cccd_md));
    
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;
    char_md.p_char_user_desc  = NULL;
    
    ble_uuid.type = uuid_type;
    ble_uuid.uuid = ESS_CHAR_UUID;
    
    memset(&attr_md, 0, sizeof(attr_md));
    
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vlen       = 0;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    
    /* 2) The Characteristic Value Attribute consists of the actual data */
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    
    
    attr_char_value.p_uuid    = &ble_uuid; //the type for this attribute is always the same UUID found in the characteristic's declaration value field
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = init_char_len;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = max_char_len;
    attr_char_value.p_value   = fake_data_p;
    
    err_code = sd_ble_gatts_characteristic_add(p_ess->ess_service_handle,
                                              &char_md,
                                              &attr_char_value,
                                              char_handles);
    
    if (err_code != NRF_SUCCESS){ return err_code; }
    
    
    /***** If notification is enabled, add the trigger setting ***/
    //Note: Trigger setting should have been initialized in p_ess_init
    if (p_ess_init->is_notify_supported == true){
        
        ble_gatts_attr_t    trigger_des;
        ble_gatts_attr_md_t trigger_des_md;
        memset(&trigger_des, 0, sizeof(trigger_des));
        memset(&trigger_des_md, 0, sizeof(trigger_des_md));

        char_data->trigger_val_cond = char_trigger_init_data->condition;
        memcpy(char_data->trigger_val_buff, &(char_trigger_init_data->condition), 1);

        if ((char_data->trigger_val_cond == 0x01) || (char_data->trigger_val_cond == 0x02)){
            memcpy(char_data->trigger_val_buff + 1, &(char_trigger_init_data->time_interval), 3);
            trigger_des.init_len = 4;
        }
        else {
            memcpy( char_data->trigger_val_buff + 1, trigger_val, max_char_len);
            trigger_des.init_len = max_char_len + 1;
        }
        
        BLE_UUID_BLE_ASSIGN(ble_uuid, ESS_UUID_ES_TRIGGER_SETTING);
        trigger_des.p_uuid = &ble_uuid;

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&trigger_des_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&trigger_des_md.write_perm);
        trigger_des_md.vlen       = 1;
        trigger_des_md.vloc       = BLE_GATTS_VLOC_STACK;
        trigger_des_md.rd_auth    = 0;
        trigger_des_md.wr_auth    = 0;

        trigger_des.p_attr_md = &trigger_des_md;
        trigger_des.p_value = char_data->trigger_val_buff;
        
        trigger_des.init_offs = 0;
        if (MAX_TRIG_LEN > 3) trigger_des.max_len = MAX_TRIG_LEN + 1;
        else trigger_des.max_len = 4;

        err_code = sd_ble_gatts_descriptor_add(BLE_GATT_HANDLE_INVALID, &trigger_des, &(char_data->trigger_handle) );
    
    }
    
    if (err_code != NRF_SUCCESS)
    { 
        return err_code; 
    }
    
    return NRF_SUCCESS;
    
}


uint32_t ble_ess_init(ble_ess_t * p_ess, const ble_ess_init_t * p_ess_init)

{
    
    volatile uint32_t   err_code;
    ble_uuid_t ble_uuid;
    
    // Initialize service structure
    p_ess->evt_handler               = p_ess_init->evt_handler;    
    p_ess->conn_handle               = BLE_CONN_HANDLE_INVALID;
    
    // Add service
    ble_uuid128_t base_uuid = {ESS_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_ess->uuid_type);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_ess->uuid_type;
    ble_uuid.uuid = ESS_UUID_SERVICE;
    
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_ess->ess_service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    memcpy(p_ess->pressure.val_last, &(p_ess_init->init_pres_data), 4);
    memcpy(p_ess->humidity.val_last, &(p_ess_init->init_hum_data), 2);
    memcpy(p_ess->temperature.val_last, &(p_ess_init->init_temp_data), 2);
    memcpy(p_ess->lux.val_last, &(p_ess_init->init_lux_data), 2);
    memcpy(p_ess->acceleration.val_last, &(p_ess_init->init_acc_data), 1);

    //Initial data pointer
    uint8_t *init_data_ptr;
    
    /******* Add pressure characteristic *******/
    init_data_ptr = (uint8_t*)&(p_ess_init->init_pres_data);
    
    err_code = ess_char_add(p_ess, p_ess_init, ESS_UUID_PRES_CHAR, BLE_UUID_TYPE_BLE, &(p_ess->pressure), init_data_ptr, INIT_PRES_LEN, MAX_PRES_LEN, 
        &(p_ess_init->pres_trigger_data),  (uint8_t*)&(p_ess_init->pres_trigger_val_var), &(p_ess->pres_char_handles) );

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    /******* Add humidity characteristic *******/
    init_data_ptr = (uint8_t*)&(p_ess_init->init_hum_data);
    
    err_code = ess_char_add(p_ess, p_ess_init, ESS_UUID_HUM_CHAR, BLE_UUID_TYPE_BLE, &(p_ess->humidity), init_data_ptr, INIT_HUM_LEN, MAX_HUM_LEN, 
        &(p_ess_init->hum_trigger_data),  (uint8_t*)&(p_ess_init->hum_trigger_val_var), &(p_ess->hum_char_handles) );

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    /****** Add temperature characteristic *******/
    init_data_ptr = (uint8_t*)&(p_ess_init->init_temp_data);

    err_code = ess_char_add(p_ess, p_ess_init, ESS_UUID_TEMP_CHAR, BLE_UUID_TYPE_BLE,  &(p_ess->temperature), init_data_ptr, INIT_TEMP_LEN, MAX_TEMP_LEN, 
        &(p_ess_init->temp_trigger_data),  (uint8_t*)&(p_ess_init->temp_trigger_val_var), &(p_ess->temp_char_handles) );

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    /******* Add lux characteristic *******/
    init_data_ptr = (uint8_t*)&(p_ess_init->init_lux_data);
    
    err_code = ess_char_add(p_ess, p_ess_init, ESS_UUID_LUX_CHAR_SHORT, BLE_UUID_TYPE_BLE, &(p_ess->lux), init_data_ptr, INIT_LUX_LEN, MAX_LUX_LEN, 
        &(p_ess_init->lux_trigger_data),  (uint8_t*)&(p_ess_init->lux_trigger_val_var), &(p_ess->lux_char_handles) );

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    /******* Add acceleration characteristic *******/
    init_data_ptr = (uint8_t*)&(p_ess_init->init_acc_data);
    
    err_code = ess_char_add(p_ess, p_ess_init, ESS_UUID_ACC_CHAR_SHORT, BLE_UUID_TYPE_BLE, &(p_ess->acceleration), init_data_ptr, INIT_ACC_LEN, MAX_ACC_LEN, 
        &(p_ess_init->acc_trigger_data),  (uint8_t*)&(p_ess_init->acc_trigger_val_var), &(p_ess->acc_char_handles) );

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
    
}

int intcmp( uint8_t * buff_1, uint8_t * buff_2, uint16_t length, bool is_signed){

    int len = length-1;

    if (is_signed){
        
        if ((buff_1[len] >> 7) != (buff_2[len] >> 7) ){

            if ((buff_2[len] >> 7) == 0) return -1;
            else return 1;

        } 
    }

    while( len >= 0){

        if (buff_1[len] > buff_2[len]) return 1;
        else if (buff_1[len] < buff_2[len]) return -1;
        len--;

    }

    return 0;


}

uint32_t ble_ess_char_value_update(ble_ess_t * p_ess, ess_char_data_t * char_data, uint8_t  * ess_meas_val, uint16_t char_len, bool is_signed,  ble_gatts_char_handles_t * char_handles) //uint8_t * ess_meas_val, uint16_t char_len, bool is_signed)
{
    
    uint32_t err_code = NRF_SUCCESS;
    
    ble_gatts_value_t gatts_value;

    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len = char_len;
    gatts_value.offset = 0;
    gatts_value.p_value = ess_meas_val;


    // Update database
    err_code = sd_ble_gatts_value_set( p_ess->conn_handle,
                                      char_handles->value_handle,
                                      &gatts_value);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Send value if connected and notifying
    if ((p_ess->conn_handle != BLE_CONN_HANDLE_INVALID) && p_ess->is_notify_supported)
    {
        
       if( is_notification_needed(char_data->trigger_val_cond, char_data->trigger_val_buff, ess_meas_val, char_data->val_last, char_len, is_signed) ){
            
            //send the notification
            ble_gatts_hvx_params_t hvx_params;
            memset(&hvx_params, 0, sizeof(hvx_params));
            
            uint16_t len = sizeof(uint8_t);
            
            hvx_params.handle = char_handles->value_handle;
            hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.offset = 0;
            hvx_params.p_len  = &len;
            hvx_params.p_data = ess_meas_val;
            

            err_code = sd_ble_gatts_hvx(p_ess->conn_handle, &hvx_params);
        }
        
    }
    
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }       
       
    //update value
    memcpy(char_data->val_last, ess_meas_val, char_len);

    return err_code;
}


bool is_notification_needed(uint8_t condition, uint8_t * operand, uint8_t * ess_meas_val_new, uint8_t * ess_meas_val_old, uint8_t char_len, bool is_signed){

        bool notif_needed = false;
        
        if (condition == TRIG_FIXED_INTERVAL){
            notif_needed = true;
        }

        else if (condition == TRIG_NO_LESS){
            notif_needed = true;
        }

        else if (condition == TRIG_VALUE_CHANGE){
            int n = intcmp(ess_meas_val_new, ess_meas_val_old, char_len, is_signed);
            if ( n != 0){
                notif_needed = true;
            }
        }

        else {

            int n = intcmp(ess_meas_val_new, operand+1, char_len, is_signed);

            if (condition == TRIG_WHILE_LT){ 
                if (n < 0){
                    notif_needed = true;
                }
            }

            else if (condition == TRIG_WHILE_LTE){
                if (n <= 0){
                    notif_needed = true;
                }
            }

            else if (condition == TRIG_WHILE_GT){
                if (n>0){
                   notif_needed = true;
                }
            }

            else if (condition == TRIG_WHILE_GTE){
                if (n>=0){
                    notif_needed = true;
                }
            }

            else if (condition == TRIG_WHILE_E){
                if(n==0){
                    notif_needed = true;
                }
            }

            else if (condition == TRIG_WHILE_NE){
                if(n!=0){
                    notif_needed = true;
                }
            }

        }

    return notif_needed;
}