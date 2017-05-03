import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Enumeration;

import javax.comm.CommPortIdentifier;
import javax.comm.PortInUseException;
import javax.comm.SerialPort;
import javax.comm.UnsupportedCommOperationException;

public class SimpleRead {
    private static final  char[]COMMAND = {'*', 'R', 'D', 'Y', '*'};
    private static final  char[]CHECK = {'*', 'C', 'A', 'M','R','E','A','D','Y', '*'};
    private static final  char[]CHECKTX = {'*','T','x', 'R','E','A','D','Y','*'};
    private static final  char[]CHECKRX = {'*', 'R', 'x', 'R','E','A','D','Y', '*'};
    private static final int WIDTH = 320; //640;
    private static final int HEIGHT = 240; //480;


    private static CommPortIdentifier portId;
    private static CommPortIdentifier portTx;
    private static CommPortIdentifier portRx;
    InputStream inputStream,inputStreamRx,inputStreamTx;
    OutputStream outputStream,outputStreamTx,outputStreamRx;
    SerialPort serialPort,serialPortTx,serialPortRx;

    public static void main(String[] args) {
        Enumeration portList = CommPortIdentifier.getPortIdentifiers();
        Enumeration portListRx = CommPortIdentifier.getPortIdentifiers();
        Enumeration portListTx = CommPortIdentifier.getPortIdentifiers();


//        while (portListRx.hasMoreElements()) {
//            portId = (CommPortIdentifier) portList.nextElement();
//            if (portId.getPortType() == CommPortIdentifier.PORT_SERIAL) {
//                System.out.println("Port name: " + portId.getName());
//                if (portId.getName().equals("COMx")) {
//                    break;
//                }
//            }
//        }
        while (portListTx.hasMoreElements()) {
            portTx = (CommPortIdentifier) portList.nextElement();
            if (portTx.getPortType() == CommPortIdentifier.PORT_SERIAL) {
                System.out.println("Port name: " + portTx.getName());
                if (portTx.getName().equals("COM9")) {
                    System.out.println("COM9 is ready!!");
                    break;
                }
            }
        }
        while (portList.hasMoreElements()) {
            portId = (CommPortIdentifier) portList.nextElement();
            if (portId.getPortType() == CommPortIdentifier.PORT_SERIAL) {
                System.out.println("Port name: " + portId.getName());
                if (portId.getName().equals("COM4")) {
                    System.out.println("COM4 is ready!!");
                    SimpleRead reader = new SimpleRead();
                }
            }
        }

    }

    public SimpleRead() {
        int[][]rgb = new int[HEIGHT][WIDTH];
        int[][]rgb2 = new int[WIDTH][HEIGHT];



        try {
            serialPort = (SerialPort) portId.open("SimpleReadApp", 1000);
            inputStream = serialPort.getInputStream();
            outputStream = serialPort.getOutputStream();

            serialPortTx = (SerialPort) portTx.open("SimpleReadAppTX", 1000);
            outputStreamTx = serialPortTx.getOutputStream();
            inputStreamTx = serialPortTx.getInputStream();

//            serialPortRx = (SerialPort) portRx.open("SimpleReadApp", 1000);
//            inputStreamRx = serialPort.getInputStream();
//            outputStreamTx = serialPortTx.getOutputStream();

            serialPort.setSerialPortParams(1000000,
                    SerialPort.DATABITS_8,
                    SerialPort.STOPBITS_1,
                    SerialPort.PARITY_NONE);

            serialPortTx.setSerialPortParams(9600,
                    SerialPort.DATABITS_8,
                    SerialPort.STOPBITS_1,
                    SerialPort.PARITY_NONE);

//            serialPortRx.setSerialPortParams(1000000,
//                    SerialPort.DATABITS_8,
//                    SerialPort.STOPBITS_1,
//                    SerialPort.PARITY_NONE);

            while (!isReadyTx(inputStreamTx, 0)) {
                System.out.println("Tx doesn't ready!!");
            }
            ;
            System.out.println("Tx is ready!!");

//            while(!isReadyRx(inputStreamTx, 0)){
//                System.out.println("Tx doesn't ready!!");
//            };
//            System.out.println("Tx is ready!!");
            while (!(read(inputStreamTx) == '3')) {
                System.out.println("Wait.....");
            }
            outputStream.write('L');

            int counter = 0;
            int numW = 0;
            int numH = 0;
            int numSum = 0;
            ArrayList Data = new ArrayList();

            while (!isReady(inputStream, 0)) {
                System.out.println("Camera doesn't ready!!");
            }
            ;
            System.out.println("Camera is ready!!");

            while (counter < 6) {
                System.out.println("Looking for image");

                while (!isImageStart(inputStream, 0)) {
                }
                ;

                System.out.println("Found image: " + counter);

                numH = 0;
                numSum = 0;
                for (int y = 0; y < HEIGHT; y++) {
                    numW = 0;
                    for (int x = 0; x < WIDTH; x++) {
                        int temp = read(inputStream);
                        rgb[y][x] = ((temp & 0xFF) << 16) | ((temp & 0xFF) << ￼ | (temp & 0xFF);
                        numW += temp;
                    }
                    numH += (numW / WIDTH);
                }
                numSum = (numH / HEIGHT);
                Data.add(counter, numSum);
                System.out.println(numSum);

                for (int y = 0; y < HEIGHT; y++) {

                    for (int x = 0; x < WIDTH; x++) {
                        rgb2[x][y] = rgb[y][x];
                    }
                }

                BMP bmp = new BMP();
                bmp.saveBMP("C:/new/out" + (counter++) + " " + numSum + ".bmp", rgb2);

                System.out.println("Saved image: " + counter);
            }

            System.out.println("Calibating");

            String text;
            int earth = Integer.parseInt(Data.get(5).toString());
            int ulti = Integer.parseInt(Data.get(4).toString());
            int louis = Integer.parseInt(Data.get(3).toString());
            text = ((earth < 100 ? earth < 10 ? "00" : "0" : "") + Data.get(5).toString() + " " + (ulti < 100 ? ulti < 10 ? "00" : "0" : "") + Data.get(4).toString() + " " + (louis < 100 ? louis < 10 ? "00" : "0" : "") + Data.get(3).toString());
            System.out.println(text);

            for (int i = 0; i < text.length(); i++) {
                outputStreamTx.write(text.charAt(i));
                System.out.println((text.charAt(i)));
            }
            String CC = "";
            int tmp;


            while(true)
            {
                CC="";

            while (true) {
                tmp = read(inputStreamTx);
                if (tmp == '#') {
                    break;
                } else {
                    CC += ((char) tmp);
                }

            }
            System.out.println("*" + CC.charAt(0) + "*");
            outputStream.write(CC.charAt(0));
            if(CC.charAt(0)=='3')
            {
//                outputStream.write(CC.charAt(0));
                outputStream.write('L');
            }
//            outputStream.write(CC.charAt(0));
            CC = CC.substring(2, CC.length());
            int tmpp = Integer.parseInt(CC);
            System.out.println(tmpp);

            while (true) {
                System.out.println("Looking for image");

                while (!isImageStart(inputStream, 0)) {
                }
                ;

                System.out.println("Found image: " + counter);
                int dummy = 0;
                for (int i = 0; i < tmpp; i++) {
                    int tmpH = (int) (Math.random() * 256);
                    int tmpW = (int) (Math.random() * 241);
                    for (int y = 0; y < tmpH; y++) {

                        for (int x = 0; x < tmpW; x++) {
                            if ((y == tmpH - 1) && (x == tmpW - 1))
                                dummy = read(inputStream);
                        }

                    }
                    String dummyy = "";
                    dummyy += (tmpW + " " + tmpH + " " + dummy + " ");
                    int in = 0;
                    while (in < dummyy.length()) {
                        outputStreamTx.write(dummyy.charAt(in));
                        in++;
                    }
                }

                BMP bmp = new BMP();
                bmp.saveBMP("C:/new/out" + (counter++) + " " + numSum + ".bmp", rgb2);

                System.out.println("Saved image: " + counter);
                break;
            }
        }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private int read(InputStream inputStream) throws IOException {
        int temp = (char) inputStream.read();
        if (temp == -1) {
            throw new  IllegalStateException("Exit");
        }
        return temp;
    }

    private boolean isImageStart(InputStream inputStream, int index) throws IOException {
        if (index < COMMAND.length) {
            if (COMMAND[index] == read(inputStream)) {
                return isImageStart(inputStream, ++index);
            } else {
                return false;
            }
        }
        return true;
    }
    private boolean isReady(InputStream inputStream, int index) throws IOException {
        if (index < CHECK.length) {
            if (CHECK[index] == read(inputStream)) {
                return isReady(inputStream, ++index);
            } else {
                return false;
            }
        }
        return true;
    }
    private boolean isReadyTx(InputStream inputStream, int index) throws IOException {
        if (index < CHECKTX.length) {
            if (CHECKTX[index] == read(inputStream)) {
                return isReadyTx(inputStream, ++index);
            } else {
                return false;
            }
        }
        return true;
    }
    private boolean isReadyRx(InputStream inputStream, int index) throws IOException {
        if (index < CHECKRX.length) {
            if (CHECKRX[index] == read(inputStream)) {
                return isReadyRx(inputStream, ++index);
            } else {
                return false;
            }
        }
        return true;
    }
}
