<script setup lang="ts">
// vue 3 + vite use MQTT.js refer to https://github.com/mqttjs/MQTT.js/issues/1269
import * as mqtt from "mqtt/dist/mqtt.min";
import { reactive, ref, onMounted } from "vue";
import * as echarts from "echarts";
import axios from "axios";
// https://github.com/mqttjs/MQTT.js#qos
const qosList = [0, 1, 2];

/**
 * this demo uses EMQX Public MQTT Broker (https://www.emqx.com/en/mqtt/public-mqtt5-broker), here are the details:
 *
 * Broker host: broker.emqx.io
 * WebSocket port: 8083
 * WebSocket over TLS/SSL port: 8084
 */
const connection = reactive({
  // ws or wss
  protocol: "ws",
  host: "broker.emqx.io",
  // ws -> 8083; wss -> 8084
  port: 8083,
  clientId: "emqx_vue3_" + Math.random().toString(16).substring(2, 8),
  /**
   * By default, EMQX allows clients to connect without authentication.
   * https://docs.emqx.com/en/enterprise/v4.4/advanced/auth.html#anonymous-login
   */
  username: "emqx_test",
  password: "emqx_test",
  clean: true,
  connectTimeout: 30 * 1000, // ms
  reconnectPeriod: 4000, // ms
  // for more options and details, please refer to https://github.com/mqttjs/MQTT.js#mqttclientstreambuilder-options
});

// topic & QoS for MQTT subscribing
const subscription = ref({
  topic: "PUBLISH",
  qos: 0 as mqtt.QoS,
});

// topic, QoS & payload for publishing message
const publish = ref({
  topic: "subtopic",
  qos: 0 as mqtt.QoS,
  payload: `{
  "service_id": "control",
  "command_name":{
    "water":"OFF",
    "fan":"OFF",
    "auto":"OFF"
  },
  "set":{
    "HumiThreshold":50,
    "TempThreshold":30
  }
}`
});

let client = ref({
  connected: false,
} as mqtt.MqttClient);
const receivedMessages = ref("");
const connectedSuccess = ref(false);
const subscribedSuccess = ref(false);
const btnLoadingType = ref("");
const retryTimes = ref(0);

const initData = () => {
  client.value = {
    connected: false,
  } as mqtt.MqttClient;
  retryTimes.value = 0;
  btnLoadingType.value = "";
  connectedSuccess.value = false;
  subscribedSuccess.value = false;
  receivedMessages.value = '';
  fanSwitch.value = false;
  pumpSwitch.value = false;
  autoControlSwitch.value = false;
  temperatureThreshold.value = 0;
  humidityThreshold.value = 0;
  date.value = [];
  temperatureData.value = [];
  air_humidityData.value = [];
  soil_humidityData.value = [];
  echartsOption.xAxis.data = date.value;
  echartsOption.series[0].data = temperatureData.value;
  echartsOption.series[1].data = air_humidityData.value;
  echartsOption.series[2].data = soil_humidityData.value;
  myChart.setOption(echartsOption);
};

const handleOnReConnect = () => {
  retryTimes.value += 1;
  if (retryTimes.value > 5) {
    try {
      client.value.end();
      initData();
      console.log("connection maxReconnectTimes limit, stop retry");
    } catch (error) {
      console.log("handleOnReConnect catch error:", error);
    }
  }
};

// create MQTT connection
const createConnection = () => {
  try {
    btnLoadingType.value = "connect";
    const { protocol, host, port, ...options } = connection;
    const connectUrl = `${protocol}://${host}:${port}/mqtt`;

    /**
     * if protocol is "ws", connectUrl = "ws://broker.emqx.io:8083/mqtt"
     * if protocol is "wss", connectUrl = "wss://broker.emqx.io:8084/mqtt"
     * 
     * /mqtt: MQTT-WebSocket uniformly uses /path as the connection path,
     * which should be specified when connecting, and the path used on EMQX is /mqtt.
     * 
     * for more details about "mqtt.connect" method & options,
     * please refer to https://github.com/mqttjs/MQTT.js#mqttconnecturl-options
     */
    client.value = mqtt.connect(connectUrl, options);

    if (client.value.on) {
      // https://github.com/mqttjs/MQTT.js#event-connect
      client.value.on("connect", () => {
        btnLoadingType.value = "";
        console.log("connection successful");
        subscription.value.topic = 'PUBLISH';
        doUnSubscribe();
        subscription.value.topic = 'error';
        doUnSubscribe()
        subscription.value.topic = 'sht40_sensor/data_recv';
        doUnSubscribe();
        subscription.value.topic = 'PUBLISH';
        doSubscribe();//订阅预设的subtopic主题
        subscription.value.topic = 'error';
        doSubscribe();
        subscription.value.topic = 'sht40_sensor/data_recv';
        doSubscribe();
        connectedSuccess.value = true;
      });

      // https://github.com/mqttjs/MQTT.js#event-reconnect
      client.value.on("reconnect", handleOnReConnect);

      // https://github.com/mqttjs/MQTT.js#event-error
      client.value.on("error", (error) => {
        console.log("connection error:", error);
      });

      // https://github.com/mqttjs/MQTT.js#event-message
      client.value.on("message", (topic: string, message) => {
        receivedMessages.value = receivedMessages.value.concat(
          message.toString()
        );
        console.log(`received message: ${message} from topic: ${topic}`);
        try {
          const data = JSON.parse(message);
          if (
            data.temperature !== undefined &&
            data.air_humidity !== undefined &&
            data.soil_humidity !== undefined
          ) {
            // 是传感器数据，调用处理函数
            handleSensorData(message);
          }
        } catch (error) {
          // 非传感器数据，不做处理
          console.error('Received non-sensor data:', error);
        }

      });
    }

  } catch (error) {
    btnLoadingType.value = "";
    console.log("mqtt.connect error:", error);
  }
};

// subscribe topic

const doSubscribe = () => {
  btnLoadingType.value = "subscribe";
  const { topic, qos } = subscription.value;
  client.value.subscribe(
    topic,
    { qos },
    (error: Error, granted: mqtt.ISubscriptionGrant[]) => {
      btnLoadingType.value = "";
      if (error) {
        console.log("subscribe error:", error);
        return;
      }
      subscribedSuccess.value = true;
      console.log("subscribe successfully:", granted);
    }
  );
};

// unsubscribe topic

const doUnSubscribe = () => {
  btnLoadingType.value = "unsubscribe";
  const { topic, qos } = subscription.value;
  client.value.unsubscribe(topic, { qos }, (error) => {
    btnLoadingType.value = "";
    subscribedSuccess.value = false;
    if (error) {
      console.log("unsubscribe error:", error);
      return;
    }
    console.log(`unsubscribed topic: ${topic}`);
  });
};

// publish message

const doPublish = () => {
  btnLoadingType.value = "publish";
  const { topic, qos, payload } = publish.value;
  client.value.publish(topic, payload, { qos }, (error) => {
    btnLoadingType.value = "";
    if (error) {
      console.log("publish error:", error);
      return;
    }
  });
};

// disconnect

const destroyConnection = () => {
  if (client.value.connected) {
    btnLoadingType.value = "disconnect";
    try {
      client.value.end(false, () => {
        initData();
        console.log("disconnected successfully");
      });
    } catch (error) {
      btnLoadingType.value = "";
      console.log("disconnect error:", error);
    }
  }
};

const handleProtocolChange = (value: string) => {
  connection.port = value === "wss" ? 8084 : 8083;
};

let fanSwitch = ref(false);
let pumpSwitch = ref(false);
let autoControlSwitch = ref(false);
let temperatureThreshold = ref(0);
let humidityThreshold = ref(0);

const fanChanged = (val) => {
  if (val === true) {
    publish.value.payload = `{
  "service_id": "control",
  "command_name":{
    "fan":"ON"
  }
}`;
  } else {
    publish.value.payload = `{
  "service_id": "control",
  "command_name":{
    "fan":"OFF"
  }
}`;
  }
  receivedMessages.value += publish.value.payload + '\n';
  doPublish();
}

const pumpChanged = (val) => {
  if (val === true) {
    publish.value.payload = `{
  "service_id": "control",
  "command_name":{
    "water":"ON"
  }
}`;
  } else {
    publish.value.payload = `{
  "service_id": "control",
  "command_name":{
    "water":"OFF"
  }
}`;
  }
  receivedMessages.value += publish.value.payload + '\n';
  doPublish();
}

const autoControlChanged = (val) => {
  if (val === true) {
    publish.value.payload = `{
  "service_id": "control",
  "command_name":{
    "auto":"ON"
  }
}`;
  } else {
    publish.value.payload = `{
  "service_id": "control",
  "command_name":{
    "auto":"OFF"
  }
}`;
  }
  receivedMessages.value += publish.value.payload + '\n';
  doPublish();
}

const temperatureThresholdChanged = (val) => {
  publish.value.payload = `{
  "service_id": "control",
  "set":{
    "TempThreshold":${val}
  }
}`;
  receivedMessages.value += publish.value.payload + '\n';
  doPublish();
}

const humidityThresholdChanged = (val) => {
  publish.value.payload = `{
  "service_id": "control",
  "command_name":{
  },
  "set":{
    "HumiThreshold":${val}
  }
}`
  receivedMessages.value += publish.value.payload + '\n';
  doPublish();
}

const date = ref([]); // 实时时间数组
// 获取当前时间
const getTime = (ts = 0) => {
  const t = ts ? new Date(ts * 1000) : new Date();
  const h = t.getHours().toString().padStart(2, '0');
  const i = t.getMinutes().toString().padStart(2, '0');
  const s = t.getSeconds().toString().padStart(2, '0');
  return `${h}:${i}:${s}`;
};

const temperatureData = ref([]); // 实时温度数组
const air_humidityData = ref([]); // 实时空气湿度数组
const soil_humidityData = ref([]);//实时土壤湿度数组
const echartsOption = reactive({
  // 图表选项
  legend: {
    data: ['温度', '空气湿度', '土壤湿度'],
  },
  xAxis: {
    name: '时间',
    nameTextStyle: {
      fontWeight: 600,
      fontSize: 18,
    },
    type: 'category',
    boundaryGap: false,
    data: date.value,
  },
  yAxis: [
    // 第一个 yAxis，对应温度
    {
      name: '温度',
      nameTextStyle: {
        fontWeight: 600,
        fontSize: 18,
      },
      type: 'value',
      scale: true,
      boundaryGap: ['15%', '15%'],
      axisLabel: {
        interval: 'auto',
      },
    },
    {
      name: '湿度',
      nameTextStyle: {
        fontWeight: 600,
        fontSize: 18,
      },
      type: 'value',
      scale: true,
      boundaryGap: ['15%', '15%'],
      axisLabel: {
        interval: 'auto',
      },
    },
  ],
  tooltip: {
    trigger: 'axis',
    axisPointer: { type: 'cross' }
  },
  series: [
    {
      name: '温度',
      type: 'line',
      smooth: true,
      data: temperatureData.value,
      yAxisIndex: 0,
    },
    {
      name: '空气湿度',
      type: 'line',
      smooth: true,
      data: air_humidityData.value,
      yAxisIndex: 1,
    },
    {
      name: '土壤湿度',
      type: 'line',
      smooth: true,
      data: soil_humidityData.value,
      yAxisIndex: 1,
    },
  ],
});



const handleSensorData = (jsonData) => {
  const data = JSON.parse(jsonData);
  // 解析温度和湿度数据
  const temperature = parseInt(data.temperature);
  const air_humidity = parseInt(data.air_humidity);
  const soil_humidity = parseInt(data.soil_humidity);
  // 将解析后的数据添加到对应的数组中
  temperatureData.value.push(temperature);
  air_humidityData.value.push(air_humidity);
  soil_humidityData.value.push(soil_humidity);
  date.value.push(getTime(Math.round(new Date().getTime() / 1000)));
  myChart.setOption(echartsOption);//重绘
}
let myChart;

onMounted(() => {
  myChart = echarts.init(document.getElementById('myChart'), 'light');
  myChart.setOption(echartsOption);
});

</script>

<template>
  <div class="mqtt-demo">
    <el-card>
      <h1>mqtt服务器配置</h1>
      <el-form label-position="top" :model="connection">
        <el-row :gutter="20">
          <el-col :span="8">
            <el-form-item prop="protocol" label="协议">
              <el-select v-model="connection.protocol" @change="handleProtocolChange">
                <el-option label="ws://" value="ws"></el-option>
                <el-option label="wss://" value="wss"></el-option>
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item prop="host" label="域名">
              <el-input v-model="connection.host"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item prop="port" label="端口号">
              <el-input v-model.number="connection.port" type="number" placeholder="8083/8084"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item prop="clientId" label="客户端标识">
              <el-input v-model="connection.clientId"> </el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item prop="username" label="用户名">
              <el-input v-model="connection.username"></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item prop="password" label="密码">
              <el-input v-model="connection.password"></el-input>
            </el-form-item>
          </el-col>

          <el-col :span="24">
            <el-button type="primary" :disabled="client.connected" @click="createConnection"
              :loading="btnLoadingType === 'connect'">
              {{ client.connected ? "Connected" : "Connect" }}
            </el-button>

            <el-button v-if="client.connected" type="danger" @click="destroyConnection"
              :loading="btnLoadingType === 'disconnect'">
              Disconnect
            </el-button>
          </el-col>
        </el-row>
      </el-form>
    </el-card>

    <el-row :gutter="5">
      <el-col :span="16">
        <el-card style="height:100%;">
          <h1>控制面板</h1>
          <!-- 添加控制面板元素 -->
          <el-space>
            <el-text>风扇</el-text>
            <v-if connectedSuccess="false">
              <el-switch v-model="fanSwitch" inline-prompt active-text="ON" inactive-text="OFF" @change="fanChanged"
                :disabled="!connectedSuccess" />
            </v-if>

            <el-text>水泵</el-text>
            <el-switch v-model="pumpSwitch" inline-prompt active-text="ON" inactive-text="OFF" @change="pumpChanged"
              :disabled="!connectedSuccess" />
            <el-text>自动控制</el-text>
            <el-switch v-model="autoControlSwitch" inline-prompt active-text="ON" inactive-text="OFF"
              @change="autoControlChanged" :disabled="!connectedSuccess" />
          </el-space>

          <br />

          <el-text>温度阈值调节</el-text>
          <el-slider v-model="temperatureThreshold" show-input @change="temperatureThresholdChanged"
            :disabled="!connectedSuccess" />
          <el-text>湿度阈值调节</el-text>
          <el-slider v-model="humidityThreshold" show-input @change="humidityThresholdChanged"
            :disabled="!connectedSuccess" />
          <div id="myChart" style="width:100%;height:287px;margin-top: 5px;overflow: hidden;">
            <!--图表注入-->
          </div>
        </el-card>
      </el-col>
      <el-col :span="8">
        <el-card style="height:100%;">
          <h1>传输信息</h1>
          <el-input type="textarea" :rows="20" v-model="receivedMessages" readonly></el-input>
        </el-card>
      </el-col>
    </el-row>

  </div>
</template>

<style>
body {
  background-image: url(https://file.mo7.cc/api/public/bz);
}

.mqtt-demo {
  max-width: 1200px;
  margin: 32px auto 0 auto;
}

h1 {
  font-size: 16px;
  margin-top: 0;
}

.el-card {
  margin-bottom: 32px;
}

.el-card__body {
  padding: 24px;
}

.el-select {
  width: 100%;
}

.text-right {
  text-align: right;
}

.sub-btn {
  margin-top: 30px;
}</style>
