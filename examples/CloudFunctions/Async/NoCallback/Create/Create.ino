
/**
 * SYNTAX:
 *
 * CloudFunctions::create(<AsyncClient>, <GoogleCloudFunctions::Parent>, <functionId>, <GoogleCloudFunctions::Function>, <AsyncResult>);
 *
 * <AsyncClient> - The async client.
 * <GoogleCloudFunctions::Parent> - The GoogleCloudFunctions::Parent object included project Id and location name in its constructor.
 * <functionId> - The name of function to create.
 * <GoogleCloudFunctions::Function> - The GoogleCloudFunctions::Function object that holds the information of function to create.
 * <AsyncResult> - The async result (AsyncResult).
 *
 * The <functionId> is the name of function to create.
 *
 * The complete usage guidelines, please visit https://github.com/mobizt/FirebaseClient
 *
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_GIGA)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>) || defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#elif __has_include(<WiFiC3.h>) || defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif __has_include(<WiFi.h>)
#include <WiFi.h>
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

// Define the project location e.g. us-central1 or asia-northeast1 */
// https://firebase.google.com/docs/projects/locations
#define PROJECT_LOCATION "PROJECT_LOCATION"

// Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID "BUCKET-NAME.appspot.com"

void timeStatusCB(uint32_t &ts);

void printResult(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection

// ServiceAuth is required for Google Cloud Functions functions.
ServiceAuth sa_auth(timeStatusCB, FIREBASE_CLIENT_EMAIL, FIREBASE_PROJECT_ID, PRIVATE_KEY, 3000 /* expire period in seconds (<= 3600) */);

FirebaseApp app;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_GIGA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client;
#endif

using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client, getNetwork(network));

CloudFunctions cfunctions;

AsyncResult aResult_no_callback;

bool taskCompleted = false;

void setup()
{

    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
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

    initializeApp(aClient, app, getAuth(sa_auth), aResult_no_callback);

    app.getApp<CloudFunctions>(cfunctions);
}

void loop()
{
    // The async task handler should run inside the main loop
    // without blocking delay or bypassing with millis code blocks.

    // This JWT token process required for ServiceAuth and CustomAuth authentications
    JWT.loop(app.getAuth());

    app.loop();

    cfunctions.loop();

    if (app.ready() && !taskCompleted)
    {
        taskCompleted = true;

        Serial.println("Creates a new function...");

        GoogleCloudFunctions::Function function;

        GoogleCloudFunctions::BuildConfig buildConfig;
        buildConfig.entryPoint("helloHttp");
        buildConfig.runtime("nodejs20");

        GoogleCloudFunctions::Source source;
        GoogleCloudFunctions::StorageSource storageSource;
        storageSource.bucket(STORAGE_BUCKET_ID);
        storageSource.object("helloWorld.zip"); // The functions source files (zip) in the Storage bucket.
        source.storageSource(storageSource);
        buildConfig.source(source);
        function.buildConfig(buildConfig);

        GoogleCloudFunctions::ServiceConfig serviceConfig;
        serviceConfig.availableMemory("256M");
        serviceConfig.ingressSettings(GoogleCloudFunctions::ALLOW_ALL);
        serviceConfig.allTrafficOnLatestRevision(true);
        serviceConfig.maxInstanceRequestConcurrency(1);

        function.serviceConfig(serviceConfig);

        function.environment(GoogleCloudFunctions::Environment::GEN_2);

        // You can set the content of function object directly with function.setContent("your content")

        cfunctions.create(aClient, GoogleCloudFunctions::Parent(FIREBASE_PROJECT_ID, PROJECT_LOCATION), "test" /* function name */, function, aResult_no_callback);
    }

    printResult(aResult_no_callback);
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

void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
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
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
}