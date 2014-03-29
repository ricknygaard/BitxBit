public class AveragerT
{
  private final int NUM_VALUES_IN_AVERAGER = 4;
  private final int AVERAGER_INDEX_MASK    = 0x03;
  
  private long[]  samples= new long[NUM_VALUES_IN_AVERAGER];
  private long    average;
  private int     index;

  public void Reset()
  {
    index   = 0;
    average = 0;
  }

  public boolean IsValid()
  {
    return index >= NUM_VALUES_IN_AVERAGER;
  }
  
  public long GetAverage()
  {
    return average;
  }

  public boolean AddNearbyValue(long value)
  {
    long delta = average >> 2; //divide by 4
    if ((index < NUM_VALUES_IN_AVERAGER)
      || ((value > (average - delta)) && (value < (average + delta))))
    {
      return AddValue(value);
    }
  
    return false;  // The value was not added. No change to the average.
  }

  public boolean AddValue(long value)
  {  
    samples[index & AVERAGER_INDEX_MASK] = value;
    index += 1;
  
    if (index >= NUM_VALUES_IN_AVERAGER)
    {
      average = samples[0]
      + samples[1]
      + samples[2]
      + samples[3];
      
      average >>= 2;  //Divide by 4

      return true;
    }
  
    return false;
  }
}
