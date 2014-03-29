
public class LaptopTimeT 
{
  private long tReset = 0;
  
  public void Reset() 
  {
    tReset = System.nanoTime();
  }
  
  public long GetUsec()
  {
    return (System.nanoTime() - tReset) / 1000;
  }
  
  public long GetCmdTime()
  {
    return GetCmdTimeFromUsec(GetUsec());
  }
  
  public long GetCmdTimeFromUsec(long tUsec)
  {    
    return tUsec >> 2; // divide by 4
  }
}
