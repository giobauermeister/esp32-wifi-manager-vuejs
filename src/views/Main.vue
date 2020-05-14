<template>
  <div class="home">

    <b-container fluid>

      <!-- <h1 class="main-header">Main</h1> -->
    <b-card class="card-container mx-auto" header-class="main-card-header" header="WiFi Manager">

      <b-row sm="4" no-gutters style="width: 80%; margin: 0 auto; padding-bottom: 15px">
        <b-col style="">
          <div style="display: inline-block;position: relative; top: 2px;" class="led-green"
           v-bind:class="{ 'led-on': isDeviceConnected}"
           ></div>
          <p v-if="isDeviceConnected" style="margin-top: 0px; margin-bottom: 0px; display: inline-block; padding-left: 10px">Connected</p>
          <p v-else style="margin-top: 0px; margin-bottom: 0px; display: inline-block; padding-left: 10px">Not connected</p>
          <p v-if="isDeviceConnected" style="margin-top: 0px; margin-bottom: 0px; display: inline-block; padding-left: 10px">00:0a:95:9d:68:16</p>
        </b-col>
      </b-row>

      <b-input-group class="input-ssid">
        <template v-slot:prepend>
          <b-input-group-text class="input-ssid-label">SSID</b-input-group-text>
        </template>
        <b-form-input v-model="ssid"></b-form-input>
        <template v-slot:append>
          <b-dropdown right text="Scan" variant="">
            <b-dropdown-item v-on:click="setSsid(ssid)" v-for="ssid in ssidList" :key="ssid">{{ ssid }}</b-dropdown-item>
            <!-- <b-dropdown-item>GiosWifi</b-dropdown-item>
            <b-dropdown-item>113</b-dropdown-item> -->
            <div class="text-center">
              <b-spinner type="grow" variant="primary" label="Text Centered"></b-spinner>
            </div>
          </b-dropdown>
        </template>
      </b-input-group>

      <b-input-group class="input-pass">
        <template v-slot:prepend>
          <b-input-group-text class="input-pass-label">Password</b-input-group-text>
        </template>
        <b-form-input></b-form-input>
        <template v-slot:append>
          <b-button variant="" v-on:click="togglePassField">
            <b-icon v-if="togglePass" icon="eye"></b-icon>
            <b-icon v-if="!togglePass" icon="eye-slash"></b-icon>
          </b-button>
          
        </template>
      </b-input-group>

      <b-form-checkbox style="position: left; padding-bottom: 10px" v-model="toggleFixedIp" name="check-button" switch size="lg">
        Static IP
      </b-form-checkbox>

      <transition name="fade">
        
        <div v-if="toggleFixedIp">
        <b-input-group class="input-ipaddress">
        <template v-slot:prepend>
          <b-input-group-text class="input-pass-label">IP Addr</b-input-group-text>
        </template>
        <b-form-input></b-form-input>
      </b-input-group>

      <b-input-group class="input-netmask">
        <template v-slot:prepend>
          <b-input-group-text class="input-pass-label">Netmask</b-input-group-text>
        </template>
        <b-form-input></b-form-input>
      </b-input-group>

      <b-input-group class="input-gateway">
        <template v-slot:prepend>
          <b-input-group-text class="input-pass-label">Gateway</b-input-group-text>
        </template>
        <b-form-input></b-form-input>
      </b-input-group>
      </div>

      </transition>

      
      

      <b-row no-gutters style="position: absolute; bottom: 20px; width: 90%; margin: 0 auto;">
        <b-col>
          <b-button @click="$bvModal.show('modal-test-wifi')" style="min-width: 5rem">Test WiFi</b-button>
        </b-col>
        <b-col>
          <b-button @click="$bvModal.show('modal-erase')" style="min-width: 5rem">Erase</b-button>
          </b-col>
        <b-col>
          <b-button @click="$bvModal.show('modal-start')" style="min-width: 5rem">Start</b-button>
          </b-col>
      </b-row>

      

      


    </b-card>

    </b-container>

    <b-modal size="xsm" id="modal-test-wifi" centered hide-footer hide-header>
      <!-- <template v-slot:modal-title>
        <p style="font-size: 15px;">Testing WiFi Network</p>
      </template> -->
      <div class="d-block text-center">
        <p v-if="!isTestingWifi">Status message</p>
        <div class="text-center">
          <b-spinner style="width: 3rem; height: 3rem;" v-if="isTestingWifi" type="grow" variant="primary" label="Text Centered"></b-spinner>
        </div>
      </div>
      <b-button v-if="isTestingWifi" class="mt-1" block @click="$bvModal.hide('modal-test-wifi')">Cancel</b-button>
      <b-button v-if="isWifiOk" class="mt-1" block @click="$bvModal.hide('modal-test-wifi')">Ok</b-button>
    </b-modal>

    <b-modal size="xsm" id="modal-erase" centered hide-header>
      <!-- <template v-slot:modal-title>
        <p style="font-size: 15px;">Testing WiFi Network</p>
      </template> -->
      <div class="d-block text-center">
        <p v-if="!isErasing">Are you sure you want to erase current WiFi configuration?</p>
        <div class="text-center">
          <b-spinner style="width: 3rem; height: 3rem;" v-if="isErasing" type="grow" variant="primary" label="Text Centered"></b-spinner>
        </div>
      </div>
      <template v-slot:modal-footer="{ ok, cancel }">
        <!-- Emulate built in modal footer ok and cancel button actions -->
        <b-button size="sm" variant="success" @click="ok()">
          OK
        </b-button>
        <b-button size="sm" variant="danger" @click="cancel()">
          Cancel
        </b-button>
      </template>
    </b-modal>

    <b-modal size="xsm" id="modal-start" centered hide-header>
      <!-- <template v-slot:modal-title>
        <p style="font-size: 15px;">Testing WiFi Network</p>
      </template> -->
      <div class="d-block text-center">
        <p v-if="!isErasing">Are you sure you want to start your device?</p>
        <div class="text-center">
          <b-spinner style="width: 3rem; height: 3rem;" v-if="isErasing" type="grow" variant="primary" label="Text Centered"></b-spinner>
        </div>
      </div>
      <template v-slot:modal-footer="{ ok, cancel }">
        <!-- Emulate built in modal footer ok and cancel button actions -->
        <b-button size="sm" variant="success" @click="ok()">
          OK
        </b-button>
        <b-button size="sm" variant="danger" @click="cancel()">
          Cancel
        </b-button>
      </template>
    </b-modal>

    
  </div>
</template>

<script>
// @ is an alias to /src
// import HelloWorld from '@/components/HelloWorld.vue'

export default {
  name: "Main",
  mounted() {
    console.log("page mounted");
    const socket = new WebSocket('ws://192.168.4.1:81/', ['arduino']);
    socket.binaryType = 'arraybuffer';

    socket.onopen = function () {
      console.log('Connect ' + new Date());
      socketSendMessageInfo("getConnectStatus");
    };   

    socket.onclose = function(e) {
      console.log('Socket is closed', e.reason);
    }

    socket.onerror = function(err) {
      console.error('Socket encountered error: ', err.message, 'Closing socket');
      socket.close();
    }

    socket.onmessage = function (e) {
      console.log('Server: ', e.data);
      var jsonData = JSON.parse(e.data);
      console.log(Object.keys(jsonData)[0]);
      switch (Object.keys(jsonData)[0]) {
        case "ssidArray":
          break;
      }
    }

    function socketSendMessageInfo(message) {
      var JsonData = {
        "messageInfo": {
          "message": message,
          "size": null
        }
      }
      JsonData.messageInfo.size = JsonData.messageInfo.message.length.toString();
      socket.send(JSON.stringify(JsonData));
    }

  },
  components: {},
  data() {
    return {
      togglePass: false,
      toggleFixedIp: false,
      isDeviceConnected: true,
      isSearchingWifi: true,
      isTestingWifi: true,
      isWifiOk: false,
      ssid: "",
      ssidList: ["GiosWifi", "113", "Directa"],
    };
  },
  methods: {
    togglePassField: function() {
      this.togglePass = !this.togglePass;
      this.isDeviceConnected = !this.isDeviceConnected;
    },
    setSsid: function(ssid) {
      console.log(ssid);
      this.ssid = ssid;
    },
  }
};
</script>

<style scoped>
/* .home {
  min-height: 100%;
} */
/* .btn:focus {
  outline: none;
  box-shadow: none;
} */
.input-ssid {
  padding-bottom: 15px;
  max-width: 21rem;
  margin: 0 auto;
}
.input-pass {
  padding-bottom: 10px;
  max-width: 21rem;
  margin: 0 auto;
}
.input-ipaddress {
  padding-bottom: 15px;
  max-width: 21rem;
  margin: 0 auto;
}
.input-netmask {
  padding-bottom: 15px;
  max-width: 21rem;
  margin: 0 auto;
}
.input-gateway {
  padding-bottom: 15px;
  max-width: 21rem;
  margin: 0 auto;
}
.fullscreen {
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  overflow: auto;
  background: lime; /* Just to visualize the extent */
}
.main-header {
  font-variation-settings: "wght" 200, "ital" 1;
  color: black;
}
.card-container {
  max-width: 26rem;
  height: 30rem;
  color: #dbe1e7;
  background-color: #2f4564;
  margin-top: 5rem;
}
.card-body {
  padding-top: 8px!important;
}
.main-card-header {
  background-color: #223555;
  font-variation-settings: "wght" 400;
  font-size: 20px;
}
.fade-enter-active, .fade-leave-active {
  transition: opacity .3s;
}
.fade-enter, .fade-leave-to /* .fade-leave-active em versões anteriores a 2.1.8 */ {
  opacity: 0;
}
.led-box {
  height: 30px;
  width: 25%;
  margin: 10px 0;
  float: left;
}
.led-green {
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background-color: rgb(119, 136, 84);
  box-shadow: rgba(163, 44, 44, 0.2) 0 -1px 7px 1px, inset #476902 0 -1px 9px, rgb(41, 77, 0) 0 1px 6px;
}
.led-on {
  background-color: #ABFF00;
  box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #304701 0 -1px 9px, #89FF00 0 2px 12px;
}
/* .led-off {
  background-color: rgb(119, 136, 84);
  box-shadow: rgba(0, 0, 0, 0.2) 0 -1px 7px 1px, inset #304701 0 -1px 9px, rgb(41, 77, 0) 0 2px 12px;
} */
/* @media (min-width: 1px) {
  .modal .modal-xsm {
    max-width: 2rem !important;
    width: 50% !important;
  }
} */
</style>

<style>
div.modal .modal-dialog.modal-xsm {
  max-width: 300px!important;
  margin: auto;
}
</style>
