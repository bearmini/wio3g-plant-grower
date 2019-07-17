String getTagValue(WioCellular& wio, const char* tagName) {
  char url[1024] = {};
  int cx = snprintf(url, sizeof(url)-1, "http://metadata.soracom.io/v1/subscriber.tags.%s", tagName);
  if (cx < 0) {
    SerialUSB.print("snprintf failed: ");
    SerialUSB.println(cx);
    return "";
  }
  char buf[1024] = {};
  int ret = wio.HttpGet(url, buf, sizeof(buf));
  if (ret != 0) {
    SerialUSB.print("wio.HttpGet() failed: ");
    SerialUSB.println(ret);
    return "";
  }

  return buf;
}
