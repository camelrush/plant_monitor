# プランター見守りシステム

### システム概要
- プランターの水分量と、周囲の気温、湿度をセンサーで10分おきに収集し、Webページからグラフデータとして可視化する。
- プランターの水分量が規定値の割合を下回った場合は、あらかじめ登録したLINEのチャンネルに、給水を求める通知を行う。

### 成果イメージ
- グラフビュー  
  <img src="./assets/img/graph.png" width=400px/>

- LINE通知画面  
  <img src="./assets/img/notify.png" width=400px/>

- プランター  
  <img src="./assets/img/planter.jpg" width=400px/>

### システム構成

- 配線図  
  <img src="./assets/img/liner.png" width=400px/>

- システム全景  
  <img src="./assets/img/structure.png" width=400px/>

- DB(dynamodb)格納イメージ     
  <img src="./assets/img/dynamodb.png" width=400px/>

# 環境構築手順

いくつかの細かい手順は省略します。わからない場合は、[こちら](https://github.com/camelrush/mqttAirCondition)を参考にしてください。

### 1. AWS　IoT Coreでエッジデバイスを登録する
- IoT Core
  - ESP32のためのモノ（Things）を作成し、ルート証明書、デバイス証明書、デバイス秘密鍵をダウンロードする。

### 2. エッジデバイスを作成する
- 用意するもの
  - [ESP32](https://amzn.to/3LGY90G)

  - [水分センサー(土壌湿度計)](https://amzn.to/3rnCLXC)
  - [温度・湿度センサー(DHT11)](https://amzn.to/3tdqaXr)
  - [Arduinoビルド環境（Arduino IDE、USBケーブル等）](https://burariweb.info/electronic-work/arduino-development-environment-install.html)

<br>

- プログラムソース
  - Arduino IDEで、[moisture_sensor.ino](./esp32/moisture_sensor/moisture_sensor.ino)を開く。
  - IDE環境に、以下のLibraryをインストールする。
    - [NTPClient] by Fabrice Weinberg ... NTPサーバから時刻を取得する
    - [ArduinoJson] by Benoit Blanchon ... JSONデータを生成する
    - [DHT118266] by AMD16 ... 温度・湿度センサー DHT11からデータを取得する
 
    - [PubSubClient] by Nick O'Leary ... IoTCoreへのMQTTメッセージクライアント

  - ソースファイルの、以下の箇所を修正する。
    - WifiネットワークのSSID、パスワードを設定する。
      ```c
      const char* ssid     = "(set here your wifi ssid)";
      const char* password = "(set here your wifi password)";
      ```
    - AWS IoTCoreからダウンロードした、ルート証明書(Root CA 1)、デバイス証明書、デバイス秘密鍵を設定する（それぞれの、`-- BEGIN...` 〜 `... --- END xxx----` までを置き換える）
      ```c
      // Amazon Root CA 1
      static const char AWS_CERT_CA[] PROGMEM = R"EOF(
      -----BEGIN CERTIFICATE-----
      MIIDQ...

      ( set here root ca1 certificate. )

      ...68SQvvG5
      -----END CERTIFICATE-----
      )EOF";

      // Device Certificate
      static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
      -----BEGIN CERTIFICATE-----
      MIIDW...

      ( set here device certificate. )

      ..........................................Bpdgw==
      -----END CERTIFICATE-----

      )KEY";
      
      // Device Private Key
      static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
      -----BEGIN RSA PRIVATE KEY-----
      MIIEpA...

      ( set here device private key. )

      ..........................................t7SyA==
      -----END RSA PRIVATE KEY-----
      
      )KEY";
      ```
  - プログラムをESP32に書き込み、配線図の通り、各種センサーを接続する。
    - 配線図  
      <img src="./assets/img/liner.png" width=400px/>
      - 水分センサーの受信端子（青）は、GPIOピン33とする
      - 温度・湿度センサーの受信端子（オレンジ）は、GPIOピンの5とする。

### 3. エッジデバイスのセンサーデータをDBに登録する
  - DynamoDBにデータベースを作成する（例 moisture_data）

    - パーティションキー : dt（String デバイス配置場所）

    - ソートキー : dt_val（Number 登録日時を数値化した値。20230519112233など）

  - IoT Coreの `メッセージのルーティング` で受信データをDynamoDBに登録するルールを作成する。

  - MQTTテストクライアント、DynamoDBの内容をチェックし、デバイスから通知されてくるデータが、DynamoDBに登録されていくことを確認する。

    <img src="./assets/img/dynamodb.png" width=400px/>

### 4. DynamoDBの登録に応じたLINE通知を作成する

  - DynamoDBに作成したテーブルを開き、ストリーム機能を有効化する（新旧イメージ）

  - LINE Developerにチャネルを作成する。

    - LINE Developerにアクセスして、Messaging APIでプロバイダ、通知チャネルを作成する。

    - 作成後、アクセストークンを入手して控える。

  - AWS Lambdaに、LINEへ通知するための新規関数（python）を作成する。
    - ソースは、[notifyLineFunc.zip](./aws_lambda/zip/notifyLineFunc.zip)をアップロードする。

    - 設定画面の環境変数に、以下３つの変数を追加して、値を設定する。

      - access_token  : LINEアクセストークン

      - chart_url     : グラフWebページのURL（ ← 後で設定）

      - threshold     : 水不足の閾値%(例.10)

    - 設定画面のトリガーに、DynamoDBのテーブルを指定する。

    - DynamoDBへの登録内容のうち、水分量(mositure)が、前述の `threshold` を下回った時に、LINEに通知が行われることを確認する。

      <img src="./assets/img/notify.png" width=400px/>


### 5. グラフビューを作成する

  - AWS S3にバケットを作成し、[webpage](./webpage/)の内容をアップロードする

  - AWSのCloudFrontに、ディストリビューションを登録し、オリジンにS3の `index.htm` を指定する。

  - AWS Lambdaに、DynamoDBのデータを取得するための新規関数（python）を作成する。

    - ソースは、[moistureDataApi.zip](./aws_lambda/zip/moistureDataApi.zip)をアップロードする。

  - AWSのAPI Gatewayに、新規APIを作成する。

    - CORSを有効化する

    - メソッドリクエストの `URL クエリ文字列パラメータ` に `span` を追加する。

    - 統合リクエストの `マッピングテンプレート` に `application/json` を設定する
  
      <img src="./assets/img/aws_mapping.png" width=300px/>

  -  Webブラウザから、Cloud Frontに作成したディストリビューションにアクセスし、グラフが表示されることを確認する。
 
       <img src="./assets/img/graph.png" width=400px/>

