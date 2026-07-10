import React, {useRef, useEffect} from 'react';
import {ScrollView, Text, StyleSheet, View} from 'react-native';

type Props = {
  logs: string[];
};

export default function StatusLog({logs}: Props) {
  const scrollRef = useRef<ScrollView>(null);

  useEffect(() => {
    setTimeout(() => scrollRef.current?.scrollToEnd({animated: true}), 50);
  }, [logs]);

  return (
    <View style={styles.wrapper}>
      <ScrollView ref={scrollRef} style={styles.scroll}>
        {logs.map((line, i) => (
          <Text key={i} style={styles.line}>
            {line}
          </Text>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    flex: 1,
    backgroundColor: '#111',
    borderRadius: 6,
    padding: 8,
  },
  scroll: {flex: 1},
  line: {
    color: '#00E676',
    fontFamily: 'monospace',
    fontSize: 11,
    lineHeight: 17,
  },
});
