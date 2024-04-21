/**
 * SYNTAX:
 *
 * Firestore::Documents::commit(<AsyncClient>, <Firestore::Parent>, <Writes>);
 * Firestore::Documents::commit(<AsyncClient>, <Firestore::Parent>, <Writes>, <AsyncResult>);
 * Firestore::Documents::commit(<AsyncClient>, <Firestore::Parent>, <Writes>, <AsyncResultCallback>, <uid>);
 *
 * <AsyncClient> - The async client.
 * <Firestore::Parent> - The Firestore::Parent object included project Id and database Id in its constructor.
 * <Writes> - The writes to apply.
 * <AsyncResult> - The async result (AsyncResult).
 * <AsyncResultCallback> - The async result callback (AsyncResultCallback).
 * <uid> - The user specified UID of async result (optional).
 *
 * The Firebase project Id should be only the name without the firebaseio.com.
 * The Firestore database id should be (default) or empty "".
 *
 * The complete usage guidelines, please visit https://github.com/mobizt/FirebaseClient
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <FirebaseClient.h>

#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "Web_API_KEY"

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"
#define DATABASE_URL "URL"

#define FIREBASE_PROJECT_ID "PROJECT_ID"

void asyncCB(AsyncResult &aResult);

void printResult(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

FirebaseApp app;

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
#elif defined(ARDUINO_ARCH_SAMD)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client;
#endif

// In case the keyword AsyncClient using in this example was ambigous and used by other library, you can change
// it with other name with keyword "using" or use the class name AsyncClientClass directly.

using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client, getNetwork(network));

Firestore::Documents Docs;

AsyncResult aResult_no_callback;

int counter = 0;

unsigned long dataMillis = 0;

void setup()
{

    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    Serial.println("Initializing app...");

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
    ssl_client.setInsecure();
#if defined(ESP8266)
    ssl_client.setBufferSizes(4096, 1024);
#endif
#endif

    app.setCallback(asyncCB);

    initializeApp(aClient, app, getAuth(user_auth));

    // Waits for app to be authenticated.
    // For asynchronous operation, this blocking wait can be ignored by calling app.loop() in loop().
    ms = millis();
    while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000)
        ;

    app.getApp<Firestore::Documents>(Docs);
}

void loop()
{
    // This function is required for handling async operations and maintaining the authentication tasks.
    app.loop();

    // This required when different AsyncClients than used in FirebaseApp assigned to the Firestore functions.
    Docs.loop();

    // To get anyc result without callback
    // printResult(aResult_no_callback);

    if (app.ready() && (millis() - dataMillis > 60000 || dataMillis == 0))
    {
        dataMillis = millis();
        counter++;

        Serial.println("Commit a document (append array)... ");

        // test_collection is the collection id, test_document is the document id.
        String documentPath = "test_collection/test_document";
        String fieldPath = "appended_data";

        // If the document path contains space e.g. "a b c/d e f"
        // It should encode the space as %20 then the path will be "a%20b%20c/d%20e%20f"

        // array value to append
        Values::ArrayValue arrV(Values::IntegerValue((int)rand()));
        arrV.add(Values::StringValue("word don't come easy " + String(counter)));

        FieldTransform::AppendMissingElements<Values::ArrayValue> appendValue(arrV);
        FieldTransform::FieldTransform fieldTransforms(fieldPath, appendValue);
        DocumentTransform transform(documentPath, fieldTransforms);

        Writes writes(Write(transform, Precondition() /* currentDocument precondition */));

        // Another array value to append
        Values::ArrayValue arrV2(Values::DoubleValue((int)rand() * 1.234));
        arrV2.add(Values::StringValue("never gonna give you up " + String(counter)));
        // Another append array object
        FieldTransform::AppendMissingElements<Values::ArrayValue> appendValue2(arrV2);
        // Another field transform
        FieldTransform::FieldTransform fieldTransforms2(fieldPath, appendValue2);
        // Another doc transform
        DocumentTransform transform2(documentPath, fieldTransforms2);
        // Add another Write object of another transform to the Writes object
        writes.add(Write(transform2, Precondition()));

        // All Writes, Write, DocumentTransform FieldTransform::xxx, and Values::xxxx objects can be printed on Serial port

        // You can set the content of write and writes objects directly with write.setContent("your content") and writes.setContent("your content")

        Docs.commit(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), writes, asyncCB);

        // To assign UID for async result
        // Docs.commit(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), writes, asyncCB, "myUID");

        // To get anyc result without callback
        // Docs.commit(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), writes, aResult_no_callback);
    }
}

void asyncCB(AsyncResult &aResult)
{
    printResult(aResult);
}

void printResult(AsyncResult &aResult)
{
    if (aResult.appEvent().code() > 0)
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf("payload: task: %s, %s\n", aResult.uid().c_str(), aResult.c_str());
    }
}
