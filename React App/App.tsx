import { StatusBar } from 'expo-status-bar';
import React, { useEffect, useState } from 'react';
import { StyleSheet, Text, View, TouchableOpacity, Slider } from 'react-native';
// import { View, Button } from "react-native-elements";
import { Ionicons, Foundation, MaterialCommunityIcons } from "@expo/vector-icons";

enum Control {
  Heater,
  LedArray,
  Motor
}


const App = () => {
  const [heaterState, setHeaterState] = useState(false);
  const [ledState, setLedState] = useState(false);
  const [sliderValue, setSliderValue] = useState(0);
  const [oldSliderValue, setOldSliderValue] = useState(0);
  const [currentTemp, setCurrentTemp] = useState(0);

  const [ledArrayTimer, setLedArrayTimer] = useState(0);
  const [motorTimer, setMotorTimer] = useState(0);
  const [heaterTimer, setHeaterTimer] = useState(0);

  const str_pad_left = (string: string, pad: string, length: number) => {
    return (new Array(length + 1).join(pad) + string).slice(-length);
  }

  const refresh = () => {
    fetch('http://192.168.4.1/', {
      method: 'GET',
    }).then(result => {
      try {
        return result.text();
      }
      catch (error) {
        alert("Error in getting the result!");
      }
    }).then((text: any) => {
      try {
        // var str = text
        //   .split(`{`).join(`{"`)
        //   .split(`:`).join(`":`)
        //   .split(`,`).join(`,"`);
        let jsonResult = JSON.parse(text);
        setHeaterState(jsonResult.heaterEnabled == true);
        setLedState(jsonResult.ledArrayEnabled == true);
        setSliderValue(parseInt(jsonResult.motorPower));
        setOldSliderValue(parseInt(jsonResult.motorPower));
        setCurrentTemp(parseFloat(jsonResult.currentTemp));
        setHeaterTimer(parseInt(jsonResult.heaterTimer));
        setMotorTimer(parseInt(jsonResult.motorTimer));
        setLedArrayTimer(parseInt(jsonResult.ledArrayTimer));
      }
      catch (error) {
        alert("Error in parsing the result!");
      }
    }).catch(err => {
      alert("Could not fetch machine current state! Please check your connection")
    });
  }

  const getFormattedTimer = (control: Control): string => {
    let currentTime: number;
    switch (control) {
      case Control.Heater:
        currentTime = heaterTimer;
        break;
      case Control.LedArray:
        currentTime = ledArrayTimer;
        break;
      case Control.Motor:
        currentTime = motorTimer;
        break;
      default:
        currentTime = 0;
        break;
    }
    var minutes = Math.floor(currentTime / 60);
    var seconds = currentTime % 60;
    var hours = Math.floor(currentTime / 3600);

    var finalTime:string = str_pad_left(hours.toString(),'0',2)+':'+str_pad_left(minutes.toString(),'0',2)+':'+str_pad_left(seconds.toString(),'0',2);
    return finalTime;
  }

  useEffect(() => {
    setInterval(() => {
      setMotorTimer(motorTimer + 1);
      setHeaterTimer(heaterTimer + 1);
      setLedArrayTimer(ledArrayTimer + 1);
    }, 1000)
  }, [])

  return (
    <View style={styles.container}>
      <Text style={styles.title}>ABL Covid Test</Text>
      <View style={styles.ViewStyle}>
        <Text style={styles.ViewTitleStyle}>Heater</Text>
        <TouchableOpacity
          onPress={() => {
            fetch(heaterState ? 'http://192.168.4.1/heater/off' : 'http://192.168.4.1/heater/on', {
              method: 'GET',
              headers: {
                Accept: 'application/json',
                'Content-Type': 'text/html'
              },
            }).then(result => {
              alert("Heater state updated succesfully");
            }).catch(err => {
              alert("Could not update heater state, please check your connection")
            });
            setHeaterState(!heaterState);
            setHeaterTimer(0);
          }}
        >
          <Ionicons name="md-flame" size={32} color={heaterState ? "red" : "#ccc"} style={styles.buttonStyle} />
        </TouchableOpacity>
        <Text style={{ color: "white" }}>Current temp : {currentTemp}</Text>
        {heaterState && <Text style={{ color: "aqua" }}>{getFormattedTimer(Control.Heater)}</Text>}
      </View>

      <View style={styles.ViewStyle}>
        <Text style={styles.ViewTitleStyle}>LED Array</Text>
        <TouchableOpacity
          onPress={() => {
            fetch(ledState ? 'http://192.168.4.1/ledArray/off' : 'http://192.168.4.1/ledArray/on', {
              method: 'GET',
              headers: {
                Accept: 'text/html',
                'Content-Type': 'text/html'
              },
            }).then(result => {
              alert("LED array state updated succesfully");
            }).catch(err => {
              alert("Could not update LED array state, please check your connection")
            });
            setLedState(!ledState);
            setLedArrayTimer(0);
          }}
        >
          <Foundation name="lightbulb" size={32} color={ledState ? "yellow" : "#ccc"} style={styles.buttonStyle} />
        </TouchableOpacity>
        {ledState && <Text style={{ color: "aqua" }}>{getFormattedTimer(Control.LedArray)}</Text>}
      </View>

      <View style={[styles.ViewStyle, { height: 150 }]}>
        <Text style={styles.ViewTitleStyle}>Motor Speed</Text>
        <TouchableOpacity
          onPress={() => {
            fetch('http://192.168.4.1/motor/' + sliderValue, {
              method: 'GET',
              headers: {
                Accept: 'text/html',
                'Content-Type': 'text/html'
              },
            }).then(result => {
              setOldSliderValue(sliderValue);
              setMotorTimer(0);
            }).catch(err => {
              alert("Could not update motor state, please check your connection")
            });
          }}
          style={{ alignItems: "center" }}
        >
          <MaterialCommunityIcons name="engine" size={32} color={sliderValue !== 0 ? "green" : "#ccc"} style={[styles.buttonStyle, { alignContent: "center" }]} />
          {(sliderValue !== oldSliderValue) && <Text style={{ color: "white" }}>Press to update motor speed</Text>}
        </TouchableOpacity>
        <Slider
          value={sliderValue}
          maximumValue={100}
          minimumValue={0}
          style={styles.slider}
          minimumTrackTintColor="red"
          maximumTrackTintColor="white"
          thumbTintColor="red"
          step={1}
          disabled={false}
          onValueChange={(value) => {
            setSliderValue(value);
          }}
        />
        <Text style={styles.ViewTitleStyle}>{sliderValue}</Text>
        {(oldSliderValue > 0) && <Text style={{ color: "blue" }}>{getFormattedTimer(Control.Motor)}</Text>}
      </View>

      <View style={styles.ViewStyle}>
        <Text style={styles.ViewTitleStyle}>Refresh</Text>
        <TouchableOpacity
          onPress={() => {
            refresh();
          }}
        >
          <Ionicons name="md-refresh" size={32} color={"#ccc"} style={styles.buttonStyle} />
        </TouchableOpacity>
        <Text style={{ color: "white" }}>Press refresh to update</Text>
      </View>
      <StatusBar style="auto" />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#fff',
    alignItems: 'center',
    justifyContent: 'center',
  },
  ViewStyle: {
    backgroundColor: "#333",
    width: "80%",
    alignItems: "center",
    borderRadius: 5,
    height: 100,
    margin: 20,
    alignContent: "center",
    justifyContent: "center"
  },
  ViewTitleStyle: {
    color: "#ccc"
  },
  title: {
    fontSize: 32,
  },
  buttonStyle: {
    backgroundColor: "#555",
    padding: 10,
    borderRadius: 8,
    minHeight: 50,
    minWidth: 50,
    alignItems: "center",
    justifyContent: "center"
  },
  slider: {
    width: 200,
    height: 40,
  },
});

export default App;