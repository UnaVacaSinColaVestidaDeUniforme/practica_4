/*
 * Copyright 2025 NXP
 * 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <openthread/coap.h>
#include <openthread/cli.h>
#include "LED.h"
#include "Temp_sensor.h"
#include <stdio.h>
#include <stdlib.h>

#include "Coap_Server.h"
#include <ctype.h>
#include <string.h>

otInstance *instance_g;

/* Arreglo donde se guarda el valor default y se cambia el valor de nombre */
static char nombre_value[20] = "Sin nombre";

void handle_led_request(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    char payload[10];
    int length = otMessageRead(aMessage, otMessageGetOffset(aMessage), payload, sizeof(payload) - 1);
    payload[length] = '\0';

    if (payload[0] == '1')
    {
        // Turn LED on
        otCliOutputFormat("Payload Recived: %s\r\n", payload);
        otCliOutputFormat("LED On \r\n");
        LED_ON();

    }
    else if (payload[0] == '0')
    {
        // Turn LED off
        otCliOutputFormat("Payload Recived: %s\r\n", payload);
        otCliOutputFormat("LED Off \r\n");
        LED_OFF();
    }

    //Send response
    otMessage *response = otCoapNewMessage(instance_g, NULL);
    otCoapMessageInitResponse(response, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
    otCoapSendResponse(instance_g, response, aMessageInfo);
}

void handle_sensor_request(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static double temp_value = 0;
    temp_value = Get_Temperature();

    otMessage *response;

    if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)  
    {
        response = otCoapNewMessage(instance_g, NULL);
        otCliOutputFormat("GET\r\n");

        if (response != NULL)
        {
            otCoapMessageInitResponse(response, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CONTENT);
            
            otCoapMessageSetPayloadMarker(response);
           
            char sensorData[50] = {"0"};
            
            snprintf(sensorData, sizeof(sensorData), "%d", (int)temp_value);
            otCliOutputFormat("payload: %s\r\n", sensorData);

            otMessageAppend(response, sensorData, strlen(sensorData));

            otCoapSendResponse(instance_g, response, aMessageInfo);
        }
    }
}

void handle_nombre_request(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otMessage *response;

    if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET) {
        /* Crear mensaje vacío */
        response = otCoapNewMessage(instance_g, NULL);
        otCliOutputFormat("GET\r\n");                       // Imprimir GET, solo para Depurar

        /* Verificar si el mensaje se creo correctamente */
        if (response != NULL) {
            /*
                - Inciar mensaje de respuesta:
                    1. OT_COAP_TYPE_ACKNOLEDGMENT:  Sirve para indicar que el mensaje fue recibido.
                    2. OT_COAP_CODE_CONTENT:        Indica éxito, y que contiene datos.
            */
            otCoapMessageInitResponse(response, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CONTENT);
            
            /* Preparar mensaje de respuesta */
            otCoapMessageSetPayloadMarker(response);                            // Preparar la respuesta
            otCliOutputFormat("payload: %s\r\n", nombre_value);                 // Imprimir el valor de 'nombre_value'
            otMessageAppend(response, nombre_value, strlen(nombre_value));      // Concatenar todos los caracteres.

            /* Mandar respuesta */
            otCoapSendResponse(instance_g, response, aMessageInfo);
        }
    }

    if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_PUT) {
        /* Recibir el contenido del mensaje */
        char payload[20];
        int length = otMessageRead(aMessage, otMessageGetOffset(aMessage), payload, sizeof(payload) - 1);
        payload[length] = '\0';

        /* Verificar si la longitud es menor de 19 caracteres */
        bool valid = true;
        if (length >= sizeof(nombre_value)) {
            valid = false;
            otCliOutputFormat("Nombre demasiado largo (Máx 19 caracteres)\n\r");
        }
        /* Verificar si solo son letras y espacios */
        for (int i = 0; i < length; i++) {
            if (!isalpha(payload[i]) && !isspace(payload[i])) {
                valid = false;
                otCliOutputFormat("Caracter inválido en la posición %d: %s", i, payload);
            }
        }

        /* Crear mensaje vacío */
        response = otCoapNewMessage(instance_g, NULL);

        /* Verificar si el mensaje se creo correctamente, y si el mensaje recibido cumple con los requerimientos */
        if ((response != NULL) && (valid)) {
            /* Copiar el nuevo nombre al arreglo */
            strncpy(nombre_value, payload, sizeof(nombre_value) - 1);
            nombre_value[sizeof(nombre_value) - 1] = '\0';

            /*
            - Inciar mensaje de respuesta:
                1. OT_COAP_TYPE_ACKNOLEDGMENT:  Sirve para indicar que el mensaje fue recibido.
                2. OT_COAP_CODE_CHANGED:        Indica éxito, y que se cambió el contenido.
            */
            otCoapMessageInitResponse(response, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
            otCliOutputFormat("Se ha actualizado el nombre correctamente\n\r");
            
            /* Preparar mensaje de respuesta */
            otCoapMessageSetPayloadMarker(response);

            /* Mandar respuesta */
            otCoapSendResponse(instance_g, response, aMessageInfo);
        }
    }

    if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_DELETE) {
        /* Hacer que el arreglo tenga el valor predeterminado */
        strncpy(nombre_value, "Sin nombre", sizeof("Sin nombre"));
        otCliOutputFormat("Nombre restablecido a su valor por defecto\n\r");

        /* Crear mensaje vacío */
        response = otCoapNewMessage(instance_g, NULL);

        /* Solo ejecutar si 'response' se creo correctamente */
        if (response != NULL) {          
            otCoapMessageInitResponse(response, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_DELETED);
            
            /* Preparar mensaje de respuesta */
            otCoapMessageSetPayloadMarker(response);

            /* Mandar respuesta */
            otCoapSendResponse(instance_g, response, aMessageInfo);
        }
    }
}

void init_coap_server(otInstance *aInstance)
{
    I2C2_InitPins();
    LED_INIT();
    Temp_Sensor_start();

    instance_g = aInstance;
    
    static otCoapResource coapResource_led;
    static otCoapResource coapResource_sensor;
    static otCoapResource coapResource_nombre;
    
    coapResource_led.mUriPath = "led";
    coapResource_led.mHandler = handle_led_request;
    coapResource_led.mContext = NULL;
    coapResource_led.mNext = NULL;
    otCoapAddResource(aInstance, &coapResource_led);

    coapResource_sensor.mUriPath = "sensor";
    coapResource_sensor.mHandler = handle_sensor_request;
    coapResource_sensor.mContext = NULL;
    coapResource_sensor.mNext = NULL;
    otCoapAddResource(aInstance, &coapResource_sensor);

    coapResource_nombre.mUriPath = "nombre";
    coapResource_nombre.mHandler = handle_nombre_request;
    coapResource_nombre.mContext = NULL;
    coapResource_nombre.mNext = NULL;
    otCoapAddResource(aInstance, &coapResource_nombre);
}