/**
 *  Custom Device type for DuinoZigBee HA Example
 *
 *  Written by Prof. Mayhem, March 20, 2017 with code borrowed from John.Rucker(at)Solar-Current.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 */

metadata {
	definition (name: "DuinoZigbeeAPI", namespace: "Professor-Mayhem", author: "mayhem@yoyodynemonkeyworks.org") {
        capability "Refresh"
        capability "Polling"
        capability "Switch"
        
    	fingerprint endpointId: "38", profileId: "0104", deviceId: "0002", deviceVersion: "00", inClusters: "0000, 0006", outClusters: "0006"
	}

	// simulator metadata
	simulator {
    }

	// UI tile definitions
	tiles {

		standardTile("switch1", "device.switch", width: 2, height: 2, canChangeIcon: true) {
			state "off", label: '${name}', action: "Switch.on", icon: "st.switches.switch.off", backgroundColor: "#ffffff"
			state "on", label: '${name}', action: "Switch.off", icon: "st.switches.switch.on", backgroundColor: "#79b821"
		}
        
        standardTile("refresh", "device.refresh", inactiveLabel: false, decoration: "flat") {
			state "default", action:"refresh.refresh", icon:"st.secondary.refresh"
		}
        
		main ("switch1")
		details (["switch1", "refresh"])
	}
}

// Parse incoming device messages to generate events
def parse(String description) {
  log.info "parse called with --> $description"
  Map map = [:]
  if (description?.startsWith('on/off: ')) {
    map = parseOnOff(description)
  }
  else {
  	log.debug "No parse method for: $description"
  }
  if (map.value != null){
	def result = createEvent(name: map.name, value: map.value)
    log.debug "Parse returned ${result?.descriptionText}"
    return result
  }
}

private Map parseOnOff(String description) {
  Map resultMap = [:]
  if (description?.startsWith('on/off: 1')) {
    resultMap.value = "on"
  }
  else {
    resultMap.value = "off"
  }
  resultMap.name = "switch"
  resultMap.displayed = true
  return resultMap
}

// Commands to device
def configure() {
  log.debug "Binding SEP 0x38 DEP 0x01 Cluster 0x0006 Switch On/Off cluster to hub"      
  def cmd = []
  cmd << "zdo bind 0x${device.deviceNetworkId} 0x38 0x01 0x0006 {${device.zigbeeId}} {}" // Bind to end point 0x38 and the Switch Cluster
  cmd << "delay 1500"
  return cmd + refresh() // send refresh cmds as part of config
  log.info "Sending ZigBee Configuration Commands to DuinoZigBee"
}

def on() {
  log.debug "LED on()"
  sendEvent(name: "switch", value: "on")
  zigbee.on()
}

def off() {
  log.debug "LED off()"
  sendEvent(name: "switch", value: "off")
  zigbee.off()
}

def poll(){
  log.debug "Poll is calling refresh"
  refresh()
}

def refresh() {
  log.debug "sending refresh command"
  return zigbee.readAttribute(0x0006, 0x0000)+
  zigbee.configureReporting(0x0006, 0x0000, 0x10, 1, 600, null)
}
