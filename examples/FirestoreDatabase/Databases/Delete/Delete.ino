/**
 * SYNTAX:
 *
 * Firestore::Databases::deleteDatabase(<AsyncClient>, <Firestore::Parent>, <etag>);
 * Firestore::Databases::deleteDatabase(<AsyncClient>, <Firestore::Parent>, <etag>, <AsyncResult>);
 * Firestore::Databases::deleteDatabase(<AsyncClient>, <Firestore::Parent>, <etag>, <AsyncResultCallback>, <uid>);
 *
 * <AsyncClient> - The async client.
 * <Firestore::Parent> - The Firestore::Parent object included project Id and database Id in its constructor.
 * <etag> - The current etag of the Database.
 * <AsyncResult> - The async result (AsyncResult).
 * <AsyncResultCallback> - The async result callback (AsyncResultCallback).
 * <uid> - The user specified UID of async result (optional).
 *
 * The Firebase project Id should be only the name without the firebaseio.com.
 * The Firestore database id is the id of database to delete.
 *
 * If an etag is provided and does not match the current etag of the database,
 * deletion will be blocked and a FAILED_PRECONDITION error will be returned.
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

/**
 * This information can be taken from the service account JSON file.
 *
 * To download service account file, from the Firebase console, goto project settings,
 * select "Service accounts" tab and click at "Generate new private key" button
 */
#define FIREBASE_PROJECT_ID "PROJECT_ID"
#define FIREBASE_CLIENT_EMAIL "CLIENT_EMAIL"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----XXXXXXXXXXXX-----END PRIVATE KEY-----\n";

void timeStatusCB(uint32_t &ts);

void asyncCB(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection

// ServiceAuth is required for Databases functions.
ServiceAuth sa_auth(timeStatusCB, FIREBASE_CLIENT_EMAIL, FIREBASE_PROJECT_ID, PRIVATE_KEY, 3000 /* expire period in seconds (<= 3600) */);

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

Firestore::Databases Databases;

AsyncResult aResult_no_callback;

bool taskCompleted = false;

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

    initializeApp(aClient, app, getAuth(sa_auth));

    // Waits for app to be authenticated.
    // For asynchronous operation, this blocking wait can be ignored by calling app.loop() in loop().
    ms = millis();
    while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000)
    {
        // This JWT token process required for ServiceAuth and CustomAuth authentications
        JWT.loop(app.getAuth());
    }

    app.getApp<Firestore::Databases>(Databases);
}

void loop()
{
    // This JWT token process required for ServiceAuth and CustomAuth authentications
    JWT.loop(app.getAuth());

    // This function is required for handling and maintaining the authentication tasks.
    app.loop();

    // To get the authentication time to live in seconds before expired.
    // app.ttl();

    // This required when different AsyncClients than used in FirebaseApp assigned to the Firestore functions.
    Databases.loop();

    // To get anyc result without callback
    // printResult(aResult_no_callback);

    if (app.ready() && !taskCompleted)
    {
        taskCompleted = true;

        Serial.println("Deletes a database... ");

        String etag;

        Databases.deleteDatabase(aClient, Firestore::Parent(FIREBASE_PROJECT_ID, "myDb" /* database Id */), etag, asyncCB);

        // To assign UID for async result
        // Databases.deleteDatabase(aClient, Firestore::Parent(FIREBASE_PROJECT_ID, "myDb"), etag, asyncCB, "myUID");

        // To get anyc result without callback
        // Databases.deleteDatabase(aClient, Firestore::Parent(FIREBASE_PROJECT_ID, "myDb"), etag, aResult_no_callback);
    }
}

void timeStatusCB(uint32_t &ts)
{
#if defined(ESP8266) || defined(ESP32) || defined(CORE_ARDUINO_PICO)
    if (time(nullptr) < FIREBASE_DEFAULT_TS)
    {

        configTime(3 * 3600, 0, "pool.ntp.org");
        while (time(nullptr) < FIREBASE_DEFAULT_TS)
        {
            delay(100);
        }
    }
    ts = time(nullptr);
#elif __has_include(<WiFiNINA.h>) || __has_include(<WiFi101.h>)
    ts = WiFi.getTime();
#endif
}

void asyncCB(AsyncResult &aResult)
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
