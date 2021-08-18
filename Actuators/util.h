template<typename T, typename B>
void arrayAdd(T& arr, int n, B element)
{
  for (int i = 0; i<n; i++)
  {
    if (arr[i] == '\0')
    {
      arr[i] = element;
      return;
    }
  }
  //Serial.print("ELEMENT NOT ADDED TO ARRAY\n");
  return;
}

template<typename T>
void arrayRemove(T& arr, int n, int index)
{
  for(int i = index; i<n - 1; i++)
  {
    arr[i] = arr[i+1];
  }
  arr[n - 1] = '\0';
  return;
}
